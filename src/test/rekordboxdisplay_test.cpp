#include <gtest/gtest.h>
#include <kaitai/kaitaistream.h>
#include <QElapsedTimer>
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
