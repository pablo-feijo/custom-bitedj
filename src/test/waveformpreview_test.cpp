#include <gtest/gtest.h>
#include <QDomDocument>
#include <QPainter>
#include <QHideEvent>
#include <QMouseEvent>
#include "waveform/widgets/nonglwaveformwidgetabstract.h"
#include "widget/wwaveformviewer.h"

#include "control/controlobject.h"
#include "library/tabledelegates/previewbuttondelegate.h"
#include "test/mixxxtest.h"
#include "track/track.h"
#include "track/globaltrackcache.h"
#include "waveform/renderers/waveformpreviewrenderer.h"
#include "waveform/waveformwidgetfactory.h"
#include "waveform/widgets/waveformwidgettype.h"
#include "widget/woverview.h"

class WaveformRenderingTest : public MixxxTest {
  protected:
    static WaveformPointer summary(int frames = 64) {
        auto waveform = WaveformPointer::create(44100, frames - 1, 44100, -1);
        for (int i = 0; i < waveform->getDataSize(); ++i) {
            waveform->data()[i].filtered = {180, 90, 45, 220};
        }
        waveform->setCompletion(waveform->getDataSize());
        return waveform;
    }
    WaveformSignalColors colors() {
        QDomDocument doc;
        doc.setContent(QStringLiteral("<Visual><BiteDJPalette>true</BiteDJPalette>"
                "<SignalColor>#32323c</SignalColor><SignalLowColor>#004ee4</SignalLowColor>"
                "<SignalMidColor>#c95a00</SignalMidColor><SignalHighColor>#f6e9d3</SignalHighColor>"
                "</Visual>"));
        WaveformSignalColors result;
        result.setup(doc.documentElement(), SkinContext(config(), "test"));
        return result;
    }
    static bool columnHasInk(const QImage& image, int x) {
        for (int y = 0; y < image.height(); ++y) {
            if (qAlpha(image.pixel(x, y))) return true;
        }
        return false;
    }
    // Exercise the actual incremental deck image without cue/menu paint overlays.
    static void configure(WOverview& overview, const WaveformSignalColors& colors) { overview.m_signalColors = colors; }
    static QImage source(const WOverview& overview) { return overview.m_waveformSourceImage; }
    static int completion(const WOverview& overview) { return overview.m_actualCompletion; }
    static void style(WOverview& overview, int value) { overview.slotTypeControlChanged(value); }
    static QImage progressOverlay(WOverview& overview, int mode) {
        overview.m_waveformSourceImage = QImage(100, 10, QImage::Format_ARGB32_Premultiplied);
        overview.m_waveformSourceImage.fill(Qt::white);
        overview.m_waveformImageScaled = overview.m_waveformSourceImage;
        overview.m_playedOverlayColor = QColor(255, 0, 255, 255);
        overview.m_iPlayPos = 40;
        overview.setTimeDisplayMode(mode);
        QImage image(100, 10, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        overview.drawPlayedOverlay(&painter);
        return image;
    }
    static QString timeText(const WOverview& overview) {
        return overview.displayedTimeText();
    }
};

TEST_F(WaveformRenderingTest, OverviewProgressAndWatermarkMirrorDeckTimeMode) {
    ControlObject elapsed(ConfigKey("[Channel1]", "time_elapsed"));
    ControlObject remaining(ConfigKey("[Channel1]", "time_remaining"));
    elapsed.set(12);
    remaining.set(48);
    WOverview overview("[Channel1]", nullptr, config());

    const auto elapsedOverlay = progressOverlay(overview, 0);
    EXPECT_EQ(elapsedOverlay.pixelColor(10, 5), QColor(Qt::magenta));
    EXPECT_EQ(elapsedOverlay.pixelColor(90, 5), QColor(Qt::transparent));
    EXPECT_EQ(timeText(overview), QStringLiteral("0:12"));

    const auto remainingOverlay = progressOverlay(overview, 1);
    EXPECT_EQ(remainingOverlay.pixelColor(10, 5), QColor(Qt::transparent));
    EXPECT_EQ(remainingOverlay.pixelColor(90, 5), QColor(Qt::magenta));
    EXPECT_EQ(timeText(overview), QStringLiteral("-0:48"));
}

TEST_F(WaveformRenderingTest, EmptyAndUnpublishedDataDoNotRender) {
    auto wave = summary();
    wave->setCompletion(0);
    EXPECT_TRUE(WaveformPreviewRenderer::render(wave, {64, 22}, 2, colors()).isNull());
    wave->setCompletion(1); // Half of a stereo frame is not published.
    EXPECT_TRUE(WaveformPreviewRenderer::render(wave, {64, 22}, 2, colors()).isNull());
    EXPECT_TRUE(WaveformPreviewRenderer::render({}, {64, 22}, 2, colors()).isNull());
    EXPECT_TRUE(WaveformPreviewRenderer::render(summary(), {0, 22}, 2, colors()).isNull());
}

TEST_F(WaveformRenderingTest, PartialSummaryKeepsFullTrackCoordinatesInEveryStyle) {
    auto wave = summary();
    const int frames = wave->getDataSize() / 2;
    wave->setCompletion(frames); // Half the stereo frames.
    for (int type = 0; type < 4; ++type) {
        const auto image = WaveformPreviewRenderer::render(wave, {frames, 22}, type, colors());
        ASSERT_FALSE(image.isNull());
        EXPECT_TRUE(columnHasInk(image, 0));
        EXPECT_TRUE(columnHasInk(image, frames / 2 - 1));
        EXPECT_FALSE(columnHasInk(image, frames / 2));
        EXPECT_FALSE(columnHasInk(image, frames - 1));
    }
    wave->setCompletion(wave->getDataSize());
    EXPECT_TRUE(columnHasInk(WaveformPreviewRenderer::render(wave, {frames, 22}, 2, colors()), frames - 1));
}

TEST_F(WaveformRenderingTest, DownsamplingPreservesTransientBetweenPixelStarts) {
    auto wave = summary();
    for (int i = 0; i < wave->getDataSize(); ++i) wave->data()[i].m_i = 0;
    wave->data()[6].filtered = {255, 0, 0, 255};
    const auto image = WaveformPreviewRenderer::render(wave, {4, 22}, 2, colors());
    EXPECT_TRUE(columnHasInk(image, 0));
    EXPECT_FALSE(columnHasInk(image, 1));
}

TEST_F(WaveformRenderingTest, PaletteRoundTripChangesAllThreeStylesWithoutReload) {
    auto palette = colors();
    for (int type : {0, 2, 3}) {
        const auto before = WaveformPreviewRenderer::render(summary(), {64, 38}, type, palette);
        palette.applyBiteDJPalette(1);
        const auto amber = WaveformPreviewRenderer::render(summary(), {64, 38}, type, palette);
        EXPECT_NE(before, amber);
        palette.applyBiteDJPalette(0);
        EXPECT_EQ(before, WaveformPreviewRenderer::render(summary(), {64, 38}, type, palette));
    }
}

TEST_F(WaveformRenderingTest, StackedBandsHaveHighAtBottomAndLowAtTop) {
    const auto image = WaveformPreviewRenderer::render(summary(), {64, 100}, 3, colors());
    EXPECT_EQ(image.pixelColor(0, 98), QColor(Qt::blue));
    EXPECT_EQ(image.pixelColor(0, 75), QColor(Qt::green));
    EXPECT_EQ(image.pixelColor(0, 10), QColor(Qt::red));
}

TEST_F(WaveformRenderingTest, RenderingIsDeterministicAndReadOnly) {
    auto wave = summary();
    const auto completionBefore = wave->getCompletion();
    const auto first = WaveformPreviewRenderer::render(wave, {96, 38}, 2, colors());
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(first, WaveformPreviewRenderer::render(wave, {96, 38}, 2, colors()));
    }
    EXPECT_EQ(wave->getCompletion(), completionBefore);
    EXPECT_EQ(wave->getLow(0), 180);
}

TEST_F(WaveformRenderingTest, ScrollingTypesMapToTheSameSummaryStyles) {
    EXPECT_EQ(WaveformWidgetType::overviewType(WaveformWidgetType::AllShaderRGBWaveform), 2);
    EXPECT_EQ(WaveformWidgetType::overviewType(WaveformWidgetType::AllShaderFilteredWaveform), 0);
    EXPECT_EQ(WaveformWidgetType::overviewType(WaveformWidgetType::AllShaderRGBStackedWaveform), 3);
    EXPECT_EQ(WaveformWidgetType::overviewType(WaveformWidgetType::AllShaderHSVWaveform), 1);
    EXPECT_EQ(WaveformWidgetType::overviewType(WaveformWidgetType::AllShaderTexturedRGB), 2);
    EXPECT_EQ(WaveformWidgetType::overviewType(WaveformWidgetType::AllShaderTexturedStacked), 3);
}

TEST_F(WaveformRenderingTest, DeckLoadingUsesNewSummaryAndStyleResetRestartsPartialDrawing) {
    WaveformWidgetFactory::createInstance();
    {
        ControlObject palette(ConfigKey("[BiteDJ]", "waveform_palette"));
        ControlObject samples(ConfigKey("[Channel1]", "track_samples"));
        samples.set(999999); // Old deck controls deliberately disagree with new data.
        WOverview overview("[Channel1]", nullptr, config());
        overview.resize(256, 38);
        configure(overview, colors());
        auto first = Track::newTemporary();
        auto firstWave = summary(64);
        first->setWaveformSummary(firstWave);
        overview.slotLoadingTrack(first, {});
        ASSERT_FALSE(source(overview).isNull());
        EXPECT_EQ(source(overview).width(), firstWave->getDataSize() / 2);
        overview.slotTrackLoaded(first);
        for (int value : {0, 2, 3}) {
            style(overview, value);
            const auto original = source(overview);
            palette.set(1);
            EXPECT_NE(source(overview), original);
            palette.set(0);
            EXPECT_EQ(source(overview), original);
        }
        auto next = Track::newTemporary();
        auto nextWave = summary(32);
        nextWave->setCompletion(nextWave->getDataSize() / 2);
        next->setWaveformSummary(nextWave);
        overview.slotLoadingTrack(next, first);
        EXPECT_EQ(source(overview).width(), nextWave->getDataSize() / 2);
        EXPECT_EQ(completion(overview), nextWave->getCompletion());
        for (int value : {3, 2, 0}) {
            style(overview, value);
            EXPECT_EQ(completion(overview), nextWave->getCompletion());
            EXPECT_TRUE(columnHasInk(source(overview), 0));
            EXPECT_FALSE(columnHasInk(source(overview), nextWave->getDataSize() / 2 - 1));
        }
        nextWave->setCompletion(nextWave->getDataSize());
        overview.onTrackAnalyzerProgress(next->getId(), kAnalyzerProgressDone);
        EXPECT_TRUE(columnHasInk(source(overview), nextWave->getDataSize() / 2 - 1));
        next->setWaveformSummary({});
        EXPECT_TRUE(source(overview).isNull());
    }
    WaveformWidgetFactory::destroy();
}

#include <QAbstractTableModel>
#include "test/librarytest.h"
#include "widget/wlibrarytableview.h"

namespace {
class PreviewTable : public WLibraryTableView {
  public:
    explicit PreviewTable(UserSettingsPointer config) : WLibraryTableView(nullptr, config) {}
    void onShow() override {}
    bool hasFocus() const override { return QWidget::hasFocus(); }
    QString getModelStateKey() const override { return "preview-test"; }
};

class PreviewModel : public QAbstractTableModel, public TrackModel {
  public:
    PreviewModel() : TrackModel({}, "preview-test") {}
    QStringList locations{"/preview/a.wav", "/preview/b.wav"};
    mutable int loads = 0;
    int rowCount(const QModelIndex& = {}) const override { return locations.size(); }
    int columnCount(const QModelIndex& = {}) const override { return 1; }
    QVariant data(const QModelIndex&, int) const override { return {}; }
    TrackPointer getTrack(const QModelIndex&) const override { ++loads; return {}; }
    TrackPointer getTrackByRef(const TrackRef&) const override { ++loads; return {}; }
    TrackId getTrackId(const QModelIndex&) const override { ++loads; return {}; }
    QString getTrackLocation(const QModelIndex& index) const override { return locations[index.row()]; }
    QUrl getTrackUrl(const QModelIndex& index) const override { return QUrl::fromLocalFile(getTrackLocation(index)); }
    CoverInfo getCoverInfo(const QModelIndex&) const override { return {}; }
    const QVector<int> getTrackRows(TrackId) const override { return {}; }
    void search(const QString&) override {}
    const QString currentSearch() const override { return {}; }
    bool isColumnInternal(int) override { return false; }
    bool isColumnHiddenByDefault(int) override { return false; }
    SortColumnId sortColumnIdFromColumnIndex(int) const override { return SortColumnId::Invalid; }
    int columnIndexFromSortColumnId(SortColumnId) const override { return 0; }
    QString modelKey(bool) const override { return "preview-test"; }
    bool updateTrackGenre(Track*, const QString&) const override { return false; }
#if defined(__EXTRA_METADATA__)
    bool updateTrackMood(Track*, const QString&) const override { return false; }
#endif
};
}

class PreviewDelegateTest : public LibraryTest {
  protected:
    static void seed(PreviewButtonDelegate& delegate, const QString& location,
            const ConstWaveformPointer& waveform) {
        delegate.m_summaries.insert(location, new PreviewButtonDelegate::CachedSummary{waveform, {}});
    }
    static void configure(PreviewButtonDelegate& delegate, UserSettingsPointer config) {
        QDomDocument doc;
        doc.setContent(QStringLiteral("<Visual><BiteDJPalette>true</BiteDJPalette>"
                "<SignalColor>#32323c</SignalColor><SignalLowColor>#004ee4</SignalLowColor>"
                "<SignalMidColor>#c95a00</SignalMidColor><SignalHighColor>#f6e9d3</SignalHighColor>"
                "</Visual>"));
        delegate.m_colors.setup(doc.documentElement(), SkinContext(config, "test"));
    }
    static void refresh(PreviewButtonDelegate& delegate) { delegate.refreshVisiblePreviews(); }
    static int cacheSize(const PreviewButtonDelegate& delegate) { return delegate.m_previewCache.size(); }
    static qint64 pixmapKey(const PreviewButtonDelegate& delegate, const QString& path) {
        const auto* item = delegate.m_previewCache.object(path);
        return item ? item->pixmap.cacheKey() : 0;
    }
    static WaveformPointer wave(int low, int high) {
        auto result = WaveformPointer::create(44100, 63, 44100, -1);
        for (int i = 0; i < result->getDataSize(); ++i) {
            result->data()[i].filtered = {static_cast<unsigned char>(low), 0,
                    static_cast<unsigned char>(high), 220};
        }
        result->setCompletion(result->getDataSize());
        return result;
    }
    static QImage paint(PreviewButtonDelegate& delegate, PreviewModel& model, int row = 0) {
        QImage image(100, 30, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setPen(Qt::magenta);
        painter.setOpacity(0.8);
        const auto pen = painter.pen();
        const auto opacity = painter.opacity();
        QStyleOptionViewItem option;
        option.rect = image.rect();
        delegate.paintItem(&painter, option, model.index(row, 0));
        EXPECT_EQ(painter.pen(), pen);
        EXPECT_EQ(painter.opacity(), opacity);
        return image;
    }
};

TEST_F(PreviewDelegateTest, ReusesPixmapUntilDataProgressReplacementOrSettingsChange) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    ControlObject palette(ConfigKey("[BiteDJ]", "waveform_palette"));
    type.set(17);
    PreviewModel model;
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    auto first = wave(200, 0);
    first->setCompletion(first->getDataSize() / 2);
    seed(delegate, model.locations[0], first);
    const auto partial = paint(delegate, model);
    const auto key = pixmapKey(delegate, model.locations[0]);
    for (int i = 0; i < 50; ++i) paint(delegate, model);
    EXPECT_EQ(pixmapKey(delegate, model.locations[0]), key);
    first->setCompletion(first->getDataSize());
    const auto complete = paint(delegate, model);
    EXPECT_NE(partial, complete);
    EXPECT_NE(pixmapKey(delegate, model.locations[0]), key);
    seed(delegate, model.locations[0], wave(0, 200));
    EXPECT_NE(complete, paint(delegate, model));
    palette.set(1);
    const auto amber = paint(delegate, model);
    palette.set(0);
    EXPECT_NE(amber, paint(delegate, model));
    type.set(25);
    EXPECT_NE(complete, paint(delegate, model));
    EXPECT_EQ(model.loads, 0);
}

TEST_F(PreviewDelegateTest, ReorderedRowsStayLocationKeyedAndMissPreservesPainter) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    type.set(17);
    PreviewModel model;
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    seed(delegate, model.locations[0], wave(200, 0));
    seed(delegate, model.locations[1], wave(0, 200));
    const auto a = paint(delegate, model, 0);
    const auto b = paint(delegate, model, 1);
    EXPECT_NE(a, b);
    model.locations.swapItemsAt(0, 1);
    EXPECT_EQ(b, paint(delegate, model, 0));
    EXPECT_EQ(a, paint(delegate, model, 1));
    seed(delegate, model.locations[0], {});
    EXPECT_NE(b, paint(delegate, model, 0));
    EXPECT_EQ(model.loads, 0);
}

TEST_F(PreviewDelegateTest, BrowsingManyTracksKeepsPixmapMemoryBounded) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    type.set(17);
    PreviewModel model;
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    const auto waveform = wave(200, 0);
    for (int i = 0; i < 300; ++i) {
        model.locations[0] = QString("/preview/%1.wav").arg(i);
        seed(delegate, model.locations[0], waveform);
        paint(delegate, model);
        EXPECT_LE(cacheSize(delegate), 128);
    }
    EXPECT_EQ(model.loads, 0);
}

TEST_F(PreviewDelegateTest, ColdSummaryRemainsVisibleForMetadataOnlyTrackAndLiveClearWins) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    type.set(17);
    PreviewModel model;
    model.locations[0] = getTestDir().filePath("id3-test-data/cover-test.ogg");
    const auto track = getOrAddTrackByLocation(model.locations[0]);
    ASSERT_TRUE(track);
    track->setWaveformSummary({});
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    seed(delegate, model.locations[0], wave(200, 0));
    const auto fromDisk = paint(delegate, model);
    EXPECT_EQ(fromDisk, paint(delegate, model));
    track->setWaveformSummary(wave(0, 200));
    const auto live = paint(delegate, model);
    EXPECT_NE(fromDisk, live);
    track->setWaveformSummary({});
    const auto cleared = paint(delegate, model);
    EXPECT_NE(live, cleared);
    EXPECT_NE(fromDisk, cleared);
    EXPECT_EQ(cleared, paint(delegate, model));
}

TEST_F(PreviewDelegateTest, LoadedAnalysisReplacesMissAndSurvivesDeckUnload) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    ControlObject palette(ConfigKey("[BiteDJ]", "waveform_palette"));
    type.set(17);
    PreviewModel model;
    model.locations[0] = getTestDir().filePath("id3-test-data/cover-test.ogg");
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    seed(delegate, model.locations[0], {});
    const auto missing = paint(delegate, model);
    auto track = getOrAddTrackByLocation(model.locations[0]);
    ASSERT_TRUE(track);
    auto waveform = wave(200, 0);
    waveform->setCompletion(waveform->getDataSize() / 2);
    track->setWaveformSummary(waveform);
    // Analysis starts on another page: the table is hidden and cannot paint.
    ASSERT_FALSE(table.isVisible());
    refresh(delegate);
    const auto partial = paint(delegate, model);
    EXPECT_NE(missing, partial);
    waveform->setCompletion(waveform->getDataSize());
    refresh(delegate);
    const auto complete = paint(delegate, model);
    EXPECT_NE(partial, complete);
    track.reset();
    QCoreApplication::processEvents();
    ASSERT_FALSE(GlobalTrackCacheLocker().lookupPublishedTrackByLocation(model.locations[0]));
    EXPECT_EQ(complete, paint(delegate, model));
    palette.set(1);
    const auto amber = paint(delegate, model);
    EXPECT_NE(complete, amber);
    palette.set(0);
    EXPECT_EQ(complete, paint(delegate, model));
    // Metadata can be imported again without loading its analysis into a deck.
    track = getOrAddTrackByLocation(model.locations[0]);
    ASSERT_TRUE(track);
    track->setWaveformSummary({});
    EXPECT_EQ(complete, paint(delegate, model));
    EXPECT_EQ(model.loads, 0);
}

TEST_F(PreviewDelegateTest, LiveClearAfterPaletteInvalidationDoesNotRestoreOldSummary) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    ControlObject palette(ConfigKey("[BiteDJ]", "waveform_palette"));
    type.set(17);
    PreviewModel model;
    model.locations[0] = getTestDir().filePath("id3-test-data/cover-test.ogg");
    auto track = getOrAddTrackByLocation(model.locations[0]);
    ASSERT_TRUE(track);
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    seed(delegate, model.locations[0], wave(200, 0));
    track->setWaveformSummary(wave(0, 200));
    paint(delegate, model);
    palette.set(1);
    track->setWaveformSummary({});
    paint(delegate, model);
    EXPECT_EQ(0, pixmapKey(delegate, model.locations[0]));
    track.reset();
    QCoreApplication::processEvents();
    paint(delegate, model);
    EXPECT_EQ(0, pixmapKey(delegate, model.locations[0]));
}

// Grid mode changes interaction as well as paint: exercise the viewer with a
// lightweight renderer, without relying on a GL context or live audio engine.

class GridTestWaveform final : public NonGLWaveformWidgetAbstract {
  public:
    GridTestWaveform() : NonGLWaveformWidgetAbstract("[GridTest]", nullptr) {
        m_widget = this;
    }
    WaveformWidgetType::Type getType() const override {
        return WaveformWidgetType::QtRGBWaveform;
    }
  protected:
    void castToQWidget() override { m_widget = this; }
};

class WaveformGridEditingTest : public MixxxTest {
  protected:
    void SetUp() override { WaveformWidgetFactory::createInstance(); }
    void TearDown() override { WaveformWidgetFactory::destroy(); }
    void attach(WWaveformViewer& viewer, GridTestWaveform& waveform) {
        viewer.setWaveformWidget(&waveform);
    }
    void hide(WWaveformViewer& viewer) {
        QHideEvent event;
        viewer.hideEvent(&event);
    }
};

TEST_F(WaveformGridEditingTest, GridModeEnablesDragAndRestoresOpacityAndInteraction) {
    auto* factory = WaveformWidgetFactory::instance();
    const int previousAlpha = factory->getBeatGridAlpha();
    factory->setDisplayBeatGridAlpha(37);
    ControlObject passthrough(ConfigKey("[GridTest]", "passthrough"));
    ControlObject scratch(ConfigKey("[GridTest]", "scratch_position_enable"));
    ControlObject position(ConfigKey("[GridTest]", "scratch_position"));
    WWaveformViewer viewer("[GridTest]", config());
    GridTestWaveform waveform;
    attach(viewer, waveform);
    viewer.setSeekDisabled(true);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(30, 30), QPointF(30, 30),
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    viewer.mousePressEvent(&press);
    EXPECT_EQ(0, scratch.get());
    viewer.setGridEditMode(true);
    EXPECT_EQ(100, waveform.getBeatGridAlpha());
    viewer.mousePressEvent(&press);
    EXPECT_EQ(1, scratch.get());
    viewer.setGridEditMode(false);
    EXPECT_EQ(0, scratch.get());
    EXPECT_EQ(37, waveform.getBeatGridAlpha());
    viewer.mousePressEvent(&press);
    EXPECT_EQ(0, scratch.get());
    factory->setDisplayBeatGridAlpha(previousAlpha);
}

TEST_F(WaveformGridEditingTest, HidingGridDuringDragReleasesScratch) {
    ControlObject passthrough(ConfigKey("[GridTest]", "passthrough"));
    ControlObject scratch(ConfigKey("[GridTest]", "scratch_position_enable"));
    ControlObject position(ConfigKey("[GridTest]", "scratch_position"));
    WWaveformViewer viewer("[GridTest]", config());
    GridTestWaveform waveform;
    attach(viewer, waveform);
    viewer.setSeekDisabled(true);
    viewer.setGridEditMode(true);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(30, 30), QPointF(30, 30),
            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    viewer.mousePressEvent(&press);
    ASSERT_EQ(1, scratch.get());
    hide(viewer);
    EXPECT_EQ(0, scratch.get());
}

namespace {
class ResizeCountingLayer : public WaveformRendererAbstract {
  public:
    explicit ResizeCountingLayer(WaveformWidgetRenderer* owner)
            : WaveformRendererAbstract(owner) {}
    void setup(const QDomNode&, const SkinContext&) override {}
    void draw(QPainter*, QPaintEvent*) override {}
    void onResize() override { ++resizes; }
    int resizes = 0;
};
class ResizeTestRenderer : public WaveformWidgetRenderer {
  public:
    ResizeTestRenderer() : WaveformWidgetRenderer("[Channel1]") {
        layer = new ResizeCountingLayer(this);
        m_rendererStack.append(layer);
    }
    ResizeCountingLayer* layer;
};
}
TEST_F(WaveformRenderingTest, ReturningToUnchangedDeckViewKeepsRendererBuffers) {
    ResizeTestRenderer renderer;
    renderer.resizeRenderer(700, 200, 1.0f);
    for (int i = 0; i < 30; ++i) renderer.resizeRenderer(700, 200, 1.0f);
    EXPECT_EQ(renderer.layer->resizes, 1);
    renderer.resizeRenderer(700, 220, 1.0f);
    renderer.resizeRenderer(700, 220, 2.0f);
    EXPECT_EQ(renderer.layer->resizes, 3);
}

TEST_F(PreviewDelegateTest, ExportPreviewDoesNotFlashNativeBatchColorsOnLoad) {
    ControlObject type(ConfigKey("[Waveform]", "waveform_type"));
    type.set(17);
    PreviewModel model;
    model.locations[0] = getTestDir().filePath("id3-test-data/cover-test.ogg");
    PreviewTable table(config());
    table.setModel(&model);
    PreviewButtonDelegate delegate(&table, 0);
    configure(delegate, config());
    auto exported = wave(200, 0);
    exported->setVersion("Rekordbox browser overview");
    seed(delegate, model.locations[0], exported);
    const auto before = paint(delegate, model);
    auto track = getOrAddTrackByLocation(model.locations[0]);
    ASSERT_TRUE(track);
    track->setRekordboxWaveformSource({"/export/ANLZ.DAT", 0});
    track->setWaveformSummary(wave(0, 200));
    EXPECT_EQ(paint(delegate, model), before);
    auto deckExport = wave(100, 100);
    deckExport->setVersion("Rekordbox 3-band v3");
    track->setWaveformSummary(deckExport);
    EXPECT_NE(paint(delegate, model), before);
}

TEST_F(WaveformRenderingTest, ExportedRgbColorsIgnoreBandPaletteAndLeaveThreeBandUnchanged) {
    const auto wave = summary();
    auto palette = colors();
    const auto bands = WaveformPreviewRenderer::render(wave, {64, 38}, 0, palette);
    std::vector<WaveformRgb> rgb(wave->getDataSize() / 2, {255, 51, 25, 255, 128});
    wave->setExportedRgb(std::move(rgb));
    const auto image = WaveformPreviewRenderer::render(wave, {64, 38}, 2, palette);
    EXPECT_EQ(image.pixelColor(20, 18), QColor(255, 51, 25));
    EXPECT_EQ(bands, WaveformPreviewRenderer::render(wave, {64, 38}, 0, palette));
    palette.applyBiteDJPalette(1);
    EXPECT_EQ(image, WaveformPreviewRenderer::render(wave, {64, 38}, 2, palette));
}
