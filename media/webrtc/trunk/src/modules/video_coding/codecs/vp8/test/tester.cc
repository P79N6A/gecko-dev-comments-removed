









#include <fstream>
#include <iostream>
#include <vector>

#include "benchmark.h"
#include "dual_decoder_test.h"
#include "gtest/gtest.h"
#include "normal_async_test.h"
#include "packet_loss_test.h"
#include "vp8_unittest.h"
#include "rps_test.h"
#include "testsupport/fileutils.h"
#include "vp8.h"

using namespace webrtc;

void PopulateTests(std::vector<CodecTest*>* tests)
{

    tests->push_back(new VP8UnitTest());




}

TEST(Vp8WrapperTest, RunAllTests)
{
    VP8Encoder* enc;
    VP8Decoder* dec;
    std::vector<CodecTest*> tests;
    PopulateTests(&tests);
    std::fstream log;
    std::string log_file = webrtc::test::OutputPath() + "VP8_test_log.txt";
    log.open(log_file.c_str(), std::fstream::out | std::fstream::app);
    std::vector<CodecTest*>::iterator it;
    for (it = tests.begin() ; it < tests.end(); it++)
    {
        enc = VP8Encoder::Create();
        dec = VP8Decoder::Create();
        (*it)->SetEncoder(enc);
        (*it)->SetDecoder(dec);
        (*it)->SetLog(&log);
        (*it)->Perform();
        (*it)->Print();
        delete enc;
        delete dec;
        delete *it;
    }
   log.close();
   tests.pop_back();
}
