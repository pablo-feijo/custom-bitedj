#include <gtest/gtest.h>
#include <kaitai/kaitaistream.h>
#include <QElapsedTimer>
#include <QTest>
#include <QDateTime>
#include "library/rekordbox/rekordboxwaveform.h"
#include "library/rekordbox/rekordboxphrases.h"
using namespace mixxx::rekordbox;
using mixxx::PhraseList;
using mixxx::Phrase;
namespace {
void u16(std::string& s,unsigned n) { s+=char(n>>8); s+=char(n); }
void u32(std::string& s,unsigned n) { u16(s,n>>16); u16(s,n); }
std::string fixture(int mood, int kind, int start=1, int end=4, int fill=0, bool masked=false) {
    std::string body;
    u16(body,mood); body+=std::string(6,0); u16(body,end); body+=std::string(4,0);
    u16(body,1); u16(body,start); u16(body,kind);
    body+=std::string(14,0); body+=char(0); body+=char(fill?1:0); u16(body,fill);
    if (masked) {
        const unsigned char mask[]={203,225,238,250,229,238,173,238,233,210,233,235,225,233,243,232,233,244,225};
        for (size_t i=0;i<body.size();++i) body[i]^=char(mask[i%19]+1);
    }
    std::string result; u32(result,24); u16(result,1); return result+body;
}
PhraseList decode(std::string bytes, std::vector<double> beats={100,600,1200,1900}, int offset=0, double duration=3) {
    kaitai::kstream stream(bytes);
    rekordbox_anlz_t::song_structure_tag_t tag(&stream);
    return decodePhrases(tag,beats,duration,offset);
}
} // namespace
TEST(RekordboxDisplayTest, MonoBandsNormalizationOffsetAndNoHeldTail) {
    const std::string samples{"\x0a\x14\x1e\x28\x32\x3c",6};
    const auto wave=decodeThreeBandWaveform(samples,150,150,3,0);
    ASSERT_EQ(wave.size(),6u);
    EXPECT_EQ(wave[0].filtered.mid,10);
    EXPECT_EQ(wave[0].filtered.high,20);
    EXPECT_EQ(wave[0].filtered.low,30);
    EXPECT_EQ(wave[0].m_i,wave[1].m_i);
    EXPECT_EQ(wave[4].m_i,0);
    const auto normalized=decodeThreeBandWaveform(samples,150,150,3,0,true);
    EXPECT_EQ(normalized[2].filtered.low,255);
    EXPECT_EQ(normalized[2].filtered.mid,170);
    EXPECT_EQ(normalized[0].filtered.low,128);
    const auto shifted=decodeThreeBandWaveform(samples,150,150,3,7);
    EXPECT_EQ(shifted[0].m_i,wave[2].m_i);
    EXPECT_EQ(shifted[2].m_i,0);
    const auto silence=decodeThreeBandWaveform(std::string(6,0),150,150,2,0,true);
    EXPECT_EQ(silence[0].m_i,0);
    EXPECT_THROW(decodeThreeBandWaveform("xx",150,150,3,0),std::runtime_error);
    EXPECT_THROW(decodeThreeBandWaveform(samples,0,150,3,0),std::runtime_error);
    EXPECT_THROW(decodeThreeBandWaveform(samples,150,150,4320003,0),std::runtime_error);
}
TEST(RekordboxDisplayTest, VariableTempoMaskedPhrasesAndInvalidBoundaries) {
    const auto plain=decode(fixture(2,9,1,4,3));
    ASSERT_EQ(plain.size(),1);
    EXPECT_EQ(plain[0].kind,Phrase::Kind::Chorus);
    EXPECT_DOUBLE_EQ(plain[0].startSeconds,.1);
    EXPECT_DOUBLE_EQ(plain[0].endSeconds,1.9);
    EXPECT_DOUBLE_EQ(plain[0].fillSeconds,1.2);
    EXPECT_EQ(plain,decode(fixture(2,9,1,4,3,true)));
    EXPECT_DOUBLE_EQ(decode(fixture(2,1),{100,600,1200,1900},50)[0].startSeconds,.05);
    EXPECT_EQ(decode(fixture(2,99))[0].kind,Phrase::Kind::Unknown);
    EXPECT_THROW(decode(fixture(2,1,0)),std::runtime_error);
    EXPECT_THROW(decode(fixture(2,1,3,2)),std::runtime_error);
    EXPECT_THROW(decode(fixture(2,1,1,9)),std::runtime_error);
    EXPECT_THROW(decode(fixture(2,1),{100,600,500}),std::runtime_error);
    EXPECT_DOUBLE_EQ(decode(fixture(2,1,1,4,5))[0].fillSeconds,-1);
    kaitai::kstream stream(fixture(2,1,1,5));
    rekordbox_anlz_t::song_structure_tag_t tag(&stream);
    EXPECT_DOUBLE_EQ(decodePhrases(tag,{100,600,1200,1900},3,0,2400)[0].endSeconds,2.4);
    EXPECT_THROW(decodePhrases(tag,{100,600,1200,1900},3,0),std::runtime_error);
}
TEST(RekordboxDisplayTest, TenMinuteEnvelopeTimingAndAllocation) {
    constexpr int columns=600*150;
    std::string envelope(columns*3,0);
    for(int i=0;i<columns;++i) envelope[i*3]=char(i%126);
    QElapsedTimer timer; timer.start();
    const auto wave=decodeThreeBandWaveform(envelope,150,150,columns+1,0,true);
    RecordProperty("decode_ms",int(timer.elapsed()));
    ASSERT_EQ(wave.size(),size_t(columns+1)*2);
    EXPECT_EQ(wave[125*2].filtered.mid,255);
    EXPECT_EQ(wave[columns*2].m_i,0);
    // Generous hang/regression bound, not a hardware performance claim.
    EXPECT_LT(timer.elapsed(),5000);
}

#include <QFile>
#include <QTemporaryDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "analyzer/analyzertrack.h"
#include "analyzer/analyzerwaveform.h"
#include "waveform/waveformfactory.h"
#include "test/mixxxtest.h"
#include "library/rekordbox/rekordboxanlz.h"
#include "track/track.h"
#include "control/controlobject.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "waveform/renderers/phrasestrip.h"

namespace {
std::string section(const std::string& type, const std::string& body, int header = 12) {
    std::string out = type;
    u32(out, header); u32(out, 12 + body.size());
    return out + body;
}
std::string waveSection(const std::string& type, int peak = 125, int stride = 3) {
    std::string body;
    u32(body, stride); u32(body, 450);
    if (type == "PWV7") u32(body, 0);
    body += std::string(450 * 3, char(peak));
    return section(type, body, type == "PWV7" ? 24 : 20);
}
void writeAnalysis(const QString& path, const std::string& sections) {
    std::string bytes = "PMAI";
    u32(bytes, 28); u32(bytes, 28 + sections.size());
    bytes += std::string(16, 0) + sections;
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(bytes.data(), bytes.size()), qint64(bytes.size()));
    ASSERT_TRUE(file.flush());
    // Give each export revision a distinct timestamp, including same-size edits.
    static qint64 revision = QDateTime::currentMSecsSinceEpoch();
    ASSERT_TRUE(file.setFileTime(QDateTime::fromMSecsSinceEpoch(++revision),
            QFileDevice::FileModificationTime));
}
class RekordboxImportTest : public MixxxTest {
  protected:
    TrackPointer track() {
        auto result = Track::newTemporary(mixxx::FileAccess(
                mixxx::FileInfo(getTestDir().filePath("sine-30.wav"))));
        result->setAudioProperties(mixxx::audio::ChannelCount(2),
                mixxx::audio::SampleRate(44100), mixxx::audio::Bitrate(),
                mixxx::Duration::fromSeconds(3));
        return result;
    }
};
}
TEST_F(RekordboxImportTest, DeckLoadKeepsCachedNativeBandsInsteadOfExportColors) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const auto dat = dir.filePath("ANLZ.DAT");
    writeAnalysis(dir.filePath("ANLZ.2EX"), waveSection("PWV6") + waveSection("PWV7"));
    config()->setValue(ConfigKey("[Library]", "AnalysisCacheOnTrackFs"), false);
    config()->setValue(ConfigKey("[Library]", "AnalysisCacheInHome"), true);
    const QString connection = dir.path();
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setDatabaseName(":memory:");
        ASSERT_TRUE(db.open());
        QSqlQuery query(db);
        ASSERT_TRUE(query.exec("CREATE TABLE track_analysis (id INTEGER PRIMARY KEY, "
                               "track_id INTEGER, type INTEGER, description TEXT, "
                               "version TEXT, data_checksum INTEGER)"));
        auto t = Track::newDummy(getTestDir().filePath("sine-30.wav"), TrackId(QVariant(17)));
        t->setAudioProperties(mixxx::audio::ChannelCount(2),
                mixxx::audio::SampleRate(44100), mixxx::audio::Bitrate(),
                mixxx::Duration::fromSeconds(3));
        auto native = WaveformPointer(new Waveform(44100, 3 * 44100, 150, -1));
        for (int i = 0; i < native->getDataSize(); ++i) {
            native->data()[i].filtered = {255, 0, 0, 255};
        }
        native->setCompletion(native->getDataSize());
        AnalysisDao dao(config());
        dao.initialize(db);
        for (const auto type : {AnalysisDao::TYPE_WAVEFORM, AnalysisDao::TYPE_WAVESUMMARY}) {
            AnalysisDao::AnalysisInfo info;
            info.trackId = t->getId();
            info.type = type;
            info.version = type == AnalysisDao::TYPE_WAVEFORM
                    ? WaveformFactory::currentWaveformVersion()
                    : WaveformFactory::currentWaveformSummaryVersion();
            info.data = native->toByteArray();
            ASSERT_TRUE(dao.saveAnalysis(&info));
        }
        // This is what selecting a Rekordbox row now schedules: no eager import
        // may replace the native summary already displayed by the library.
        t->setRekordboxWaveformSource({dat, 0});
        EXPECT_FALSE(t->getWaveform());
        EXPECT_FALSE(t->getWaveformSummary());
        AnalyzerWaveform analyzer(config(), db);
        EXPECT_FALSE(analyzer.initialize(AnalyzerTrack(t), t->getSampleRate(), 3 * 44100));
        ASSERT_TRUE(t->getWaveform());
        ASSERT_TRUE(t->getWaveformSummary());
        EXPECT_EQ(t->getWaveform()->getVersion(), WaveformFactory::currentWaveformVersion());
        EXPECT_EQ(t->getWaveformSummary()->getVersion(), WaveformFactory::currentWaveformSummaryVersion());
        EXPECT_EQ(t->getWaveform()->toByteArray(), native->toByteArray());
        EXPECT_EQ(t->getWaveformSummary()->toByteArray(), native->toByteArray());
    }
    QSqlDatabase::removeDatabase(connection);
}

TEST_F(RekordboxImportTest, DeferredExportFallbackAndInvalidExportAllowNativeAnalysis) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    config()->setValue(ConfigKey("[Library]", "AnalysisCacheOnTrackFs"), false);
    config()->setValue(ConfigKey("[Library]", "AnalysisCacheInHome"), false);
    const auto dat = dir.filePath("ANLZ.DAT");
    writeAnalysis(dir.filePath("ANLZ.2EX"), waveSection("PWV6") + waveSection("PWV7"));
    auto t = track();
    t->setRekordboxWaveformSource({dat, 0});
    AnalyzerWaveform analyzer(config(), QSqlDatabase());
    EXPECT_FALSE(analyzer.initialize(AnalyzerTrack(t), t->getSampleRate(), 3 * 44100));
    ASSERT_TRUE(t->getWaveform());
    ASSERT_TRUE(t->getWaveformSummary());
    EXPECT_EQ(t->getWaveform()->getVersion(), "Rekordbox 3-band v3");
    EXPECT_EQ(t->getWaveformSummary()->getVersion(), "Rekordbox 3-band v3");
    writeAnalysis(dir.filePath("ANLZ.2EX"), waveSection("PWV6"));
    t = track();
    t->setRekordboxWaveformSource({dat, 0});
    EXPECT_TRUE(analyzer.initialize(AnalyzerTrack(t), t->getSampleRate(), 3 * 44100));
    EXPECT_EQ(t->getWaveform()->getCompletion(), 0);
    EXPECT_EQ(t->getWaveformSummary()->getCompletion(), 0);
}

TEST_F(RekordboxImportTest, PublishPairTogetherReuseAndPreserveOnInvalidExport) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const auto path = dir.filePath("ANLZ.2EX"), dat = dir.filePath("ANLZ.DAT");
    auto t = track();
    EXPECT_TRUE(readThreeBandWaveforms(t, t->getSampleRate(), 0, dat).isEmpty());
    EXPECT_FALSE(t->getWaveform()); // absent file leaves native fallback available
    writeAnalysis(path, waveSection("PWV6", 58) + waveSection("PWV7"));
    bool bothPublished = false;
    auto connection = QObject::connect(t.get(), &Track::waveformUpdated, [&] {
        bothPublished = bool(t->getWaveform()) && bool(t->getWaveformSummary());
    });
    EXPECT_TRUE(readThreeBandWaveforms(t, t->getSampleRate(), 0, dat).isEmpty());
    EXPECT_TRUE(bothPublished);
    ASSERT_TRUE(t->getWaveform());
    ASSERT_TRUE(t->getWaveformSummary());
    const auto original = t->getWaveform(), summary = t->getWaveformSummary();
    EXPECT_EQ(original->data()[0].filtered.all, 255);
    EXPECT_EQ(summary->data()[0].filtered.all, 255);
    EXPECT_EQ(original->saveState(), Waveform::SaveState::Saved);
    EXPECT_TRUE(readThreeBandWaveforms(t, t->getSampleRate(), 0, dat).isEmpty());
    EXPECT_EQ(t->getWaveform(), original);
    for (const auto& invalid : {waveSection("PWV6"),
            waveSection("PWV6") + waveSection("PWV7", 125, 4),
            waveSection("PWV6") + waveSection("PWV7") + waveSection("PWV7"),
            std::string("PWV7")}) {
        writeAnalysis(path, invalid);
        EXPECT_FALSE(readThreeBandWaveforms(t, t->getSampleRate(), 0, dat).isEmpty());
        EXPECT_EQ(t->getWaveform(), original);
        EXPECT_EQ(t->getWaveformSummary(), summary);
    }
    writeAnalysis(path, waveSection("PWV6") + waveSection("PWV7"));
    EXPECT_TRUE(readThreeBandWaveforms(t, t->getSampleRate(), 50, dat).isEmpty());
    EXPECT_NE(t->getWaveform(), original);
    EXPECT_EQ(t->getWaveform()->data()[t->getWaveform()->getDataSize() - 1].m_i, 0);
    QObject::disconnect(connection);
}
TEST_F(RekordboxImportTest, PhraseImportTracksEditedGridAndUndoWithoutDirtyingCues) {
    QTemporaryDir dir;
    const auto dat = dir.filePath("ANLZ.DAT"), ext = dir.filePath("ANLZ.EXT");
    std::string grid(8, 0); u32(grid, 4);
    QVector<mixxx::audio::FramePos> positions;
    int i = 0;
    for (int ms : {100, 600, 1200, 1900}) {
        u16(grid, ++i); u16(grid, 12000); u32(grid, ms);
        positions.append(mixxx::audio::FramePos(ms * 44.1));
    }
    writeAnalysis(dat, section("PQTZ", grid));
    writeAnalysis(ext, section("PSSI", fixture(2, 9, 1, 4, 3, true)));
    auto t = track();
    const auto source = mixxx::Beats::fromBeatPositions(t->getSampleRate(), positions);
    ASSERT_TRUE(t->trySetBeats(source)); t->markClean();
    EXPECT_TRUE(readPhrases(t, 0, dat).isEmpty());
    EXPECT_FALSE(t->isDirty());
    const auto original = t->getPhrases();
    ASSERT_EQ(original.size(), 1);
    EXPECT_NEAR(original[0].startSeconds, .1, 1e-9);
    for (auto& pos : positions) pos += 22050;
    auto edited = mixxx::Beats::fromBeatPositions(t->getSampleRate(), positions);
    // Track groups beat edits within 800 ms into one undo action.
    QTest::qWait(850);
    ASSERT_TRUE(t->trySetBeats(edited));
    ASSERT_TRUE(t->canUndoBeatsChange());
    EXPECT_NEAR(t->getPhrases()[0].startSeconds, .6, 1e-9);
    EXPECT_NEAR(t->getPhrases()[0].fillSeconds, 1.7, 1e-9);
    t->undoBeatsChange();
    EXPECT_EQ(t->getPhrases(), original);
    writeAnalysis(ext, section("PSSI", fixture(2, 1, 1, 9)));
    EXPECT_FALSE(readPhrases(t, 0, dat).isEmpty());
    EXPECT_EQ(t->getPhrases(), original);
    ASSERT_TRUE(QFile::remove(ext));
    EXPECT_TRUE(readPhrases(t, 0, dat).isEmpty());
    EXPECT_TRUE(t->getPhrases().isEmpty());
}
TEST_F(RekordboxImportTest, NativeAndExportedWaveformsShareSecondsPerPixel) {
    const QString group("[Channel1]");
    ControlObject samples(ConfigKey(group,"track_samples"));
    ControlObject rate(ConfigKey(group,"rate_ratio"));
    ControlObject gain(ConfigKey(group,"total_gain"));
    rate.set(1); gain.set(1);
    WaveformWidgetRenderer renderer(group);
    ASSERT_TRUE(renderer.init());
    renderer.resizeRenderer(720, 192, 1);
    auto t = track();
    samples.set(3 * 44100 * 2);
    renderer.setTrack(t);
    for (double zoom : {1., 2., 4.}) {
        renderer.setZoom(zoom);
        for (int visualRate : {441, 150}) {
            t->setWaveform(WaveformPointer(new Waveform(44100, 3 * 44100, visualRate, -1)));
            renderer.onPreRender(nullptr); // invalid transport does not query vsync
            EXPECT_NEAR(renderer.getAudioSamplePerPixel(), zoom * 100, 1e-9);
            EXPECT_NEAR(renderer.getVisualSamplePerPixel() *
                    t->getWaveform()->getAudioVisualRatio(), zoom * 100, 1e-9);
        }
    }
    renderer.setTrack({}); renderer.onPreRender(nullptr);
    EXPECT_EQ(renderer.getAudioSamplePerPixel(), 0);
}
TEST_F(RekordboxImportTest, PhraseStripStaysInsideVisibleBoundsInDayAndNight) {
    ControlObject visibility(ConfigKey("[BiteDJ]", "show_phrases"));
    visibility.set(1);
    mixxx::Phrase phrase;
    phrase.kind = mixxx::Phrase::Kind::Chorus;
    phrase.label = "Chorus"; phrase.startSeconds = 0; phrase.endSeconds = 3;
    for (const QColor background : {QColor(Qt::white), QColor(Qt::black)}) {
        QImage image(200, 100, QImage::Format_ARGB32);
        image.fill(background);
        QPainter painter(&image);
        mixxx::paintPhraseStrip(painter, {phrase}, QRectF(0, 20, 200, 60),
                Qt::Horizontal, 0, 3);
        painter.end();
        EXPECT_EQ(image.pixelColor(100, 10), background);
        EXPECT_EQ(image.pixelColor(100, 90), background);
        EXPECT_EQ(image.pixelColor(150, 78), mixxx::phraseColor(phrase.kind));
        EXPECT_EQ(image.pixelColor(150, 69), background); // 10px strip only
        visibility.set(0);
        image.fill(background);
        QPainter hiddenPainter(&image);
        mixxx::paintPhraseStrip(hiddenPainter, {phrase}, QRectF(0, 20, 200, 60), Qt::Horizontal, 0, 3);
        hiddenPainter.end();
        EXPECT_EQ(image.pixelColor(150, 78), background);
        visibility.set(1);
    }
}
