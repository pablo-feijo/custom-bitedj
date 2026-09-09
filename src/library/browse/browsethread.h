/*
 * browsethread.h         (C) 2011 Tobias Rafreider
 */

#pragma once

#include <QList>
#include <QMutex>
#include <QSharedPointer>
#include <QThread>
#include <QWaitCondition>
#include <QWeakPointer>
#include <QSemaphore>
#include <QStandardItem>
#include <atomic>
#include <memory>

#include "util/fileaccess.h"

// This class is a singleton and represents a thread
// that is used to read ID3 metadata
// from a particular folder.
//
// The BrowseTableModel uses this class.
// Note: Don't call getInstance() from places
// other than the GUI thread.
class BrowseTableModel;
class BrowseThread;
class QStandardItem;

struct BrowseRowBatch {
    QList<QList<QStandardItem*>> rows;
    std::shared_ptr<QSemaphore> budget;
    ~BrowseRowBatch() {
        for (const auto& row : rows) {
            qDeleteAll(row);
        }
        if (budget) {
            budget->release();
        }
    }
};
using BrowseRowBatchPointer = std::shared_ptr<BrowseRowBatch>;
Q_DECLARE_METATYPE(BrowseRowBatchPointer)

typedef QSharedPointer<BrowseThread> BrowseThreadPointer;

class BrowseThread : public QThread {
    Q_OBJECT
  public:
    virtual ~BrowseThread();
    quint64 executePopulation(mixxx::FileAccess path, BrowseTableModel* client,
            const QString& databasePath = {}, const QString& deferredLocation = {});
    void run();
    static BrowseThreadPointer getInstanceRef();

  signals:
    void rowsAppended(BrowseRowBatchPointer, BrowseTableModel*, quint64 generation);
    void clearModel(BrowseTableModel*, quint64 generation);

  private:
    BrowseThread(QObject *parent = 0);

    void populateModel();

    QWaitCondition m_locationUpdated;
    std::atomic<bool> m_bStopThread{false};
    std::atomic<quint64> m_generation{0};
    bool m_requestPending = false;
    std::shared_ptr<QSemaphore> m_batchBudget = std::make_shared<QSemaphore>(4);

    // You must hold m_path_mutex to touch m_path or m_model_observer
    QMutex m_path_mutex;
    mixxx::FileAccess m_path;
    QString m_databasePath;
    QString m_deferredLocation;
    BrowseTableModel* m_model_observer;

    static QWeakPointer<BrowseThread> m_weakInstanceRef;
};
