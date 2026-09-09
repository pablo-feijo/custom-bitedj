#include "library/autodj/autodjfeature.h"

#include <QMenu>
#include <QtDebug>

#include "control/controlpushbutton.h"
#include "library/autodj/autodjprocessor.h"
#include "library/autodj/dlgautodj.h"
#include "library/dao/trackschema.h"
#include "library/library.h"
#include "library/parser.h"
#include "notifications/notifications.h"
#include "library/playlisttablemodel.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/trackset/crate/cratestorage.h"
#include "library/treeitem.h"
#include "moc_autodjfeature.cpp"
#include "sources/soundsourceproxy.h"
#include "track/track.h"
#include "util/clipboard.h"
#include "util/defs.h"
#include "util/dnd.h"
#include "widget/wlibrary.h"
#include "widget/wlibrarysidebar.h"

namespace {

const QString kViewName = QStringLiteral("Auto DJ");

} // namespace

namespace {
constexpr int kMaxRetrieveAttempts = 3;

    int findOrCrateAutoDjPlaylistId(PlaylistDAO& playlistDAO) {
        int playlistId = playlistDAO.getPlaylistIdFromName(AUTODJ_TABLE);
        // If the AutoDJ playlist does not exist yet then create it.
        if (playlistId < 0) {
            playlistId = playlistDAO.createPlaylist(
                    AUTODJ_TABLE, PlaylistDAO::PLHT_AUTO_DJ);
            VERIFY_OR_DEBUG_ASSERT(playlistId >= 0) {
                qWarning() << "Failed to create Auto DJ playlist!";
            }
        }
        return playlistId;
    }
} // anonymous namespace

AutoDJFeature::AutoDJFeature(Library* pLibrary,
        UserSettingsPointer pConfig,
        PlayerManagerInterface* pPlayerManager)
        : LibraryFeature(pLibrary, pConfig, QStringLiteral("autodj")),
          m_pTrackCollection(pLibrary->trackCollectionManager()->internalCollection()),
          m_playlistDao(m_pTrackCollection->getPlaylistDAO()),
          m_iAutoDJPlaylistId(findOrCrateAutoDjPlaylistId(m_playlistDao)),
          m_pAutoDJProcessor(nullptr),
          m_pSidebarModel(make_parented<TreeItemModel>(this)),
          m_pAutoDJView(nullptr),
          m_autoDjCratesDao(m_iAutoDJPlaylistId, pLibrary->trackCollectionManager(), m_pConfig) {
    qRegisterMetaType<AutoDJProcessor::AutoDJState>("AutoDJState");
    m_pAutoDJProcessor = new AutoDJProcessor(this,
            m_pConfig,
            pPlayerManager,
            pLibrary->trackCollectionManager(),
            m_iAutoDJPlaylistId);

    // Connect loadTrackToPlayer signal as a queued connection to make sure all callbacks of a
    // previous load attempt have been called #10504.
    connect(m_pAutoDJProcessor,
            &AutoDJProcessor::loadTrackToPlayer,
            this,
            &LibraryFeature::loadTrackToPlayer,
            Qt::QueuedConnection);

    m_playlistDao.setAutoDJProcessor(m_pAutoDJProcessor);
    // Register before the skin parses its toolbar bindings. The native view
    // uses proxies so skin-created placeholder controls cannot shadow these.
    m_queueView = std::make_unique<ControlObject>(
            ConfigKey(QStringLiteral("[AutoDJ]"), QStringLiteral("queue_view")));
    m_moveUp = std::make_unique<ControlPushButton>(
            ConfigKey(QStringLiteral("[AutoDJ]"), QStringLiteral("move_up")));
    m_moveDown = std::make_unique<ControlPushButton>(
            ConfigKey(QStringLiteral("[AutoDJ]"), QStringLiteral("move_down")));
    m_removeSelected = std::make_unique<ControlPushButton>(
            ConfigKey(QStringLiteral("[AutoDJ]"), QStringLiteral("remove_selected")));
    m_showQueue = std::make_unique<ControlPushButton>(
            ConfigKey(QStringLiteral("[AutoDJ]"), QStringLiteral("show_queue")));
    connect(m_showQueue.get(), &ControlPushButton::valueChanged, this, [this](double value) {
        if (value <= 0) {
            return;
        }
        activate();
        emit m_pLibrary->sidebarLeafItemActivated(title().toString());
        ControlObject::set(ConfigKey(QStringLiteral("[Sidebar]"),
                                   QStringLiteral("sidebar_visible")), 0);
    });

    // Kiosk mode suppresses Qt dialogs; report start failures in the skin.
    connect(m_pAutoDJProcessor, &AutoDJProcessor::autoDJError, this,
            [](AutoDJProcessor::AutoDJError error) {
                QString message;
                switch (error) {
                case AutoDJProcessor::ADJ_QUEUE_EMPTY:
                    message = tr("Auto Play: load both decks or add tracks using + Queue / Queue All.");
                    break;
                case AutoDJProcessor::ADJ_BOTH_DECKS_PLAYING:
                case AutoDJProcessor::ADJ_UNUSED_DECK_PLAYING:
                    message = tr("Auto Play: stop one deck before starting automatic mixing.");
                    break;
                case AutoDJProcessor::ADJ_NOT_TWO_DECKS:
                    message = tr("Auto Play: assign Deck 1 and Deck 2 to opposite crossfader sides.");
                    break;
                default:
                    return;
                }
                if (auto* notifications = Notifications::tryInstance()) {
                    notifications->publish(message, Notifications::Severity::Warning);
                }
            });

    // Keep a running queue reachable even after its last track has been loaded.
    auto updateVisibility = [this] {
        emit requestSidebarVisibility(this, isSidebarVisibleByDefault());
    };
    auto* queueModel = m_pAutoDJProcessor->getTableModel();
    connect(queueModel, &QAbstractItemModel::modelReset, this, updateVisibility);
    connect(queueModel, &QAbstractItemModel::rowsInserted, this, updateVisibility);
    connect(queueModel, &QAbstractItemModel::rowsRemoved, this, updateVisibility);
    connect(m_pAutoDJProcessor, &AutoDJProcessor::autoDJStateChanged,
            this, updateVisibility);


    // Create the "Crates" tree-item under the root item.
    std::unique_ptr<TreeItem> pRootItem = TreeItem::newRoot(this);
    m_pCratesTreeItem = pRootItem->appendChild(tr("Crates"));
    m_pCratesTreeItem->setIcon(QIcon(":/images/library/ic_library_crates.svg"));

    // Create tree-items under "Crates".
    constructCrateChildModel();

    m_pSidebarModel->setRootItem(std::move(pRootItem));

    // Be notified when the status of crates changes.
    connect(m_pTrackCollection,
            &TrackCollection::crateInserted,
            this,
            &AutoDJFeature::slotCrateChanged);
    connect(m_pTrackCollection,
            &TrackCollection::crateUpdated,
            this,
            &AutoDJFeature::slotCrateChanged);
    connect(m_pTrackCollection,
            &TrackCollection::crateDeleted,
            this,
            &AutoDJFeature::slotCrateChanged);

    m_pClearQueueAction = make_parented<QAction>(tr("Clear Auto DJ Queue"), this);
    const auto removeKeySequence =
            // TODO(XXX): Qt6 replace enum | with QKeyCombination
            QKeySequence(static_cast<int>(kHideRemoveShortcutModifier) |
                    kHideRemoveShortcutKey);
    m_pClearQueueAction->setShortcut(removeKeySequence);
    connect(m_pClearQueueAction.get(),
            &QAction::triggered,
            this,
            &AutoDJFeature::slotClearQueue);
    // Create context menu item to allow crates to be removed from AutoDJ sources.
    // onRightClickChild() gets the clicked crate's id form the sidebar model and
    // assigns it to this action's data.
    // In slotRemoveCrateFromAutoDj() we retrieve the CrateId data and finally
    // remove the crate from sources in removeCrateFromAutoDj().
    m_pRemoveCrateFromAutoDjAction =
            make_parented<QAction>(tr("Remove Crate as Track Source"), this);
    m_pRemoveCrateFromAutoDjAction->setShortcut(removeKeySequence);
    connect(m_pRemoveCrateFromAutoDjAction.get(),
            &QAction::triggered,
            this,
            &AutoDJFeature::slotRemoveCrateFromAutoDj);
}

AutoDJFeature::~AutoDJFeature() {
    delete m_pAutoDJProcessor;
}

bool AutoDJFeature::isSidebarVisibleByDefault() const {
    return m_pAutoDJProcessor->getTableModel()->rowCount() > 0 ||
            m_pAutoDJProcessor->getState() != AutoDJProcessor::ADJ_DISABLED;
}

QVariant AutoDJFeature::title() {
    return tr("Auto DJ");
}

void AutoDJFeature::bindLibraryWidget(
        WLibrary* libraryWidget,
        KeyboardEventFilter* keyboard) {
    m_pAutoDJView = new DlgAutoDJ(
            libraryWidget,
            m_pConfig,
            m_pLibrary,
            m_pAutoDJProcessor,
            keyboard);
    libraryWidget->registerView(kViewName, m_pAutoDJView);
    connect(m_pAutoDJView,
            &DlgAutoDJ::loadTrack,
            this,
            &AutoDJFeature::loadTrack);
    connect(m_pAutoDJView,
            &DlgAutoDJ::loadTrackToPlayer,
            this,
            &LibraryFeature::loadTrackToPlayer);

    connect(m_pAutoDJView,
            &DlgAutoDJ::trackSelected,
            this,
            &AutoDJFeature::trackSelected);

    // Be informed when the user wants to add another random track.
    connect(m_pAutoDJProcessor,
            &AutoDJProcessor::randomTrackRequested,
            this,
            &AutoDJFeature::slotRandomQueue);
    connect(m_pAutoDJView,
            &DlgAutoDJ::addRandomTrackButton,
            this,
            &AutoDJFeature::slotAddRandomTrack);
}

void AutoDJFeature::bindSidebarWidget(WLibrarySidebar* pSidebarWidget) {
    // store the sidebar widget pointer for later use in onRightClickChild
    m_pSidebarWidget = pSidebarWidget;
}

TreeItemModel* AutoDJFeature::sidebarModel() const {
    return m_pSidebarModel;
}

void AutoDJFeature::activate() {
    //qDebug() << "AutoDJFeature::activate()";
    emit switchToView(kViewName);
    emit disableSearch();
    emit enableCoverArtDisplay(true);
}

void AutoDJFeature::clear() {
    QMessageBox::StandardButton btn = QMessageBox::question(nullptr,
            tr("Confirmation Clear"),
            tr("Do you really want to remove all tracks from the Auto DJ queue?") +
                    tr("This can not be undone."),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
    if (btn == QMessageBox::Yes) {
            m_playlistDao.clearAutoDJQueue();
    }
}

void AutoDJFeature::paste() {
    emit pasteFromSidebar();
}

// Called by SidebarModel
void AutoDJFeature::deleteItem(const QModelIndex& index) {
    TreeItem* pSelectedItem = static_cast<TreeItem*>(index.internalPointer());
    if (!pSelectedItem || pSelectedItem == m_pCratesTreeItem) {
            return;
    }
    CrateId crateId(pSelectedItem->getData());
    removeCrateFromAutoDj(crateId);
}

// Called by deleteItem and slotRemoveCrateFromAutoDj()
void AutoDJFeature::removeCrateFromAutoDj(CrateId crateId) {
    DEBUG_ASSERT(crateId.isValid());
    // TODO Confirm dialog?
    m_pTrackCollection->updateAutoDjCrate(crateId, false);
}

bool AutoDJFeature::dropAccept(const QList<QUrl>& urls, QObject* pSource) {
    // If a track is dropped onto the Auto DJ tree node, but the track isn't in the
    // library, then add the track to the library before adding it to the
    // Auto DJ playlist.
    // pSource != nullptr it is a drop from inside Mixxx and indicates all
    // tracks already in the DB
    QList<TrackId> trackIds = m_pLibrary->trackCollectionManager()->resolveTrackIdsFromUrls(urls,
            !pSource);
    if (trackIds.isEmpty()) {
        return false;
    }

    // Return whether appendTracksToPlaylist succeeded.
    return m_playlistDao.appendTracksToPlaylist(trackIds, m_iAutoDJPlaylistId);
}

bool AutoDJFeature::dragMoveAccept(const QUrl& url) {
    return SoundSourceProxy::isUrlSupported(url) ||
            Parser::isPlaylistFilenameSupported(url.toLocalFile());
}

void AutoDJFeature::slotClearQueue() {
    clear();
}

// Add a crate to the AutoDJ sources
void AutoDJFeature::slotAddCrateToAutoDj(CrateId crateId) {
    m_pTrackCollection->updateAutoDjCrate(crateId, true);
}

void AutoDJFeature::slotRemoveCrateFromAutoDj() {
    CrateId crateId(m_pRemoveCrateFromAutoDjAction->data());
    removeCrateFromAutoDj(crateId);
}

void AutoDJFeature::slotCrateChanged(CrateId crateId) {
    Crate crate;
    if (m_pTrackCollection->crates().readCrateById(crateId, &crate) && crate.isAutoDjSource()) {
        // Crate exists and is already a source for AutoDJ
        // -> Find and update the corresponding child item
        for (int i = 0; i < m_crateList.length(); ++i) {
            if (m_crateList[i].getId() == crateId) {
                QModelIndex parentIndex = m_pSidebarModel->index(0, 0);
                QModelIndex childIndex = m_pSidebarModel->index(i, 0, parentIndex);
                m_pSidebarModel->setData(childIndex, crate.getName(), Qt::DisplayRole);
                m_crateList[i] = crate;
                return; // early exit
            }
        }
        // No child item for crate found
        // -> Create and append a new child item for this crate
        // TODO() Use here std::span to get around the heap alloctaion of
        // std::vector for a single element.
        std::vector<std::unique_ptr<TreeItem>> rows;
        rows.push_back(std::make_unique<TreeItem>(crate.getName(), crate.getId().toVariant()));
        QModelIndex parentIndex = m_pSidebarModel->index(0, 0);
        m_pSidebarModel->insertTreeItemRows(std::move(rows), m_crateList.length(), parentIndex);
        m_crateList.append(crate);
    } else {
        // Crate does not exist or is not a source for AutoDJ
        // -> Find and remove the corresponding child item
        for (int i = 0; i < m_crateList.length(); ++i) {
            if (m_crateList[i].getId() == crateId) {
                QModelIndex parentIndex = m_pSidebarModel->index(0, 0);
                m_pSidebarModel->removeRows(i, 1, parentIndex);
                m_crateList.removeAt(i);
                return; // early exit
            }
        }
    }
}

void AutoDJFeature::slotAddRandomTrack() {
    if (m_iAutoDJPlaylistId >= 0) {
        TrackPointer pRandomTrack;
        for (int failedRetrieveAttempts = 0;
                !pRandomTrack && (failedRetrieveAttempts < 2 * kMaxRetrieveAttempts); // 2 rounds
                ++failedRetrieveAttempts) {
            TrackId randomTrackId;
            if (m_crateList.isEmpty()) {
                // Fetch Track from Library since we have no assigned crates
                randomTrackId = m_autoDjCratesDao.getRandomTrackIdFromLibrary(
                        m_iAutoDJPlaylistId);
            } else {
                // Fetch track from crates.
                // We do not fall back to Library if this fails because this
                // may add banned tracks
                randomTrackId = m_autoDjCratesDao.getRandomTrackId();
            }

            if (randomTrackId.isValid()) {
                pRandomTrack = m_pLibrary->trackCollectionManager()->getTrackById(randomTrackId);
                VERIFY_OR_DEBUG_ASSERT(pRandomTrack) {
                    qWarning() << "Track does not exist:"
                            << randomTrackId;
                    continue;
                }
                if (!pRandomTrack->getFileInfo().checkFileExists()) {
                    qWarning() << "Track does not exist:"
                               << pRandomTrack->getInfo()
                               << pRandomTrack->getLocation();
                    pRandomTrack.reset();
                }
            }
        }
        if (pRandomTrack) {
            m_pTrackCollection->getPlaylistDAO().appendTrackToPlaylist(
                    pRandomTrack->getId(), m_iAutoDJPlaylistId);
            m_pAutoDJView->onShow();
            return; // success
        }
    }
    qWarning() << "Could not load random track.";
}

void AutoDJFeature::constructCrateChildModel() {
    m_crateList.clear();
    CrateSelectResult autoDjCrates(m_pTrackCollection->crates().selectAutoDjCrates(true));
    Crate crate;
    while (autoDjCrates.populateNext(&crate)) {
        // Create the TreeItem for this crate.
        m_pCratesTreeItem->appendChild(crate.getName(), crate.getId().toVariant());
        m_crateList.append(crate);
    }
}

void AutoDJFeature::onRightClick(const QPoint& globalPos) {
    QMenu menu(m_pSidebarWidget);
    menu.addAction(m_pClearQueueAction.get());
    menu.exec(globalPos);
}

void AutoDJFeature::onRightClickChild(const QPoint& globalPos,
        const QModelIndex& index) {
    TreeItem* pClickedItem = static_cast<TreeItem*>(index.internalPointer());
    QMenu menu(m_pSidebarWidget);
    if (m_pCratesTreeItem == pClickedItem) {
        // The "Crates" parent item was right-clicked.
        // Bring up the context menu.
        QMenu crateMenu(m_pSidebarWidget);
        crateMenu.setTitle(tr("Add Crate as Track Source"));
        CrateSelectResult nonAutoDjCrates(m_pTrackCollection->crates().selectAutoDjCrates(false));
        Crate crate;
        while (nonAutoDjCrates.populateNext(&crate)) {
            auto pAction = std::make_unique<QAction>(crate.getName(), &crateMenu);
            auto crateId = crate.getId();
            connect(pAction.get(), &QAction::triggered, this, [this, crateId] {
                slotAddCrateToAutoDj(crateId);
            });
            crateMenu.addAction(pAction.get());
            pAction.release();
        }
        menu.addMenu(&crateMenu);
        menu.exec(globalPos);
    } else {
        // A crate child item was right-clicked.
        // Bring up the context menu.
        m_pRemoveCrateFromAutoDjAction->setData(pClickedItem->getData()); // the selected CrateId
        menu.addAction(m_pRemoveCrateFromAutoDjAction);
        menu.exec(globalPos);
    }
}

void AutoDJFeature::slotRandomQueue(int numTracksToAdd) {
    for (int addCount = 0; addCount < numTracksToAdd; ++addCount) {
        slotAddRandomTrack();
    }
}
