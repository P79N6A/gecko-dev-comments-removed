









#include "receiver_tests.h"
#include "normal_test.h"
#include "codec_database_test.h"
#include "generic_codec_test.h"
#include "../source/event.h"
#include "media_opt_test.h"
#include "quality_modes_test.h"
#include "test_util.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32

#endif

using namespace webrtc;






int vcmMacrosTests = 0;
int vcmMacrosErrors = 0;

int ParseArguments(int argc, char **argv, CmdArgs& args)
{
    int i = 1;
    while (i < argc)
    {
        if (argv[i][0] != '-')
        {
            return -1;
        }
        switch (argv[i][1])
        {
        case 'w':
        {
            int w = atoi(argv[i+1]);
            if (w < 1)
                return -1;
            args.width = w;
            break;
        }
        case 'h':
        {
            int h = atoi(argv[i+1]);
            if (h < 1)
                return -1;
            args.height = h;
            break;
        }
        case 'b':
        {
            int b = atoi(argv[i+1]);
            if (b < 1)
                return -1;
            args.bitRate = b;
            break;
        }
        case 'f':
        {
            int f = atoi(argv[i+1]);
            if (f < 1)
                return -1;
            args.frameRate = f;
            break;
        }
        case 'c':
        {
            
            
            args.codecName = argv[i+1];
            if (strncmp(argv[i+1], "VP8", 3) == 0)
            {
                args.codecType = kVideoCodecVP8;
            }
            else if (strncmp(argv[i+1], "I420", 4) == 0)
            {
                args.codecType = kVideoCodecI420;
            }
            else
                return -1;

            break;
        }
        case 'i':
        {
            args.inputFile = argv[i+1];
            break;
        }
        case 'o':
            args.outputFile = argv[i+1];
            break;
        case 'n':
        {
            int n = atoi(argv[i+1]);
            if (n < 1)
                return -1;
            args.testNum = n;
            break;
        }
        case 'p':
        {
            args.packetLoss = atoi(argv[i+1]);
            break;
        }
        case 'r':
        {
            args.rtt = atoi(argv[i+1]);
            break;
        }
        case 'm':
        {
            args.protectionMode = atoi(argv[i+1]);
            break;
        }
        case 'e':
        {
            args.camaEnable = atoi(argv[i+1]);
            break;
        }
        default:
            return -1;
        }
        i += 2;
    }
    return 0;
}

int main(int argc, char **argv)
{
    CmdArgs args;

    if (ParseArguments(argc, argv, args) != 0)
    {
        printf("Unable to parse input arguments\n");
        printf("args: -n <test #> -w <width> -h <height> -f <fps> -b <bps> "
               "-c <codec>  -i <input file> -o <output file> -p <packet loss> "
               "-r <round-trip-time> -e <cama enable> -m <protection mode> \n");
        return -1;
    }

    int ret = 0;
    switch (args.testNum)
    {
    case 1:
        ret = NormalTest::RunTest(args);
        break;
    case 2:
        ret = MTRxTxTest(args);
        break;
    case 3:
        ret = GenericCodecTest::RunTest(args);
        break;
    case 4:
        ret = CodecDataBaseTest::RunTest(args);
        break;
    case 5:
        
        ret = MediaOptTest::RunTest(0, args);
        break;
    case 6:
        ret = ReceiverTimingTests(args);
        break;
    case 7:
        ret = RtpPlay(args);
        break;
    case 8:
        ret = RtpPlayMT(args);
        break;
    case 9:
        ret = JitterBufferTest(args);
        break;
    case 10:
        ret = DecodeFromStorageTest(args);
        break;
    case 11:
        ret = NormalTest::RunTest(args);
        ret |= CodecDataBaseTest::RunTest(args);
        ret |= ReceiverTimingTests(args);
        ret |= JitterBufferTest(args);
        break;
    default:
        ret = -1;
        break;
    }
    if (ret != 0)
    {
        printf("Test failed!\n");
        return -1;
    }
    return 0;
}



