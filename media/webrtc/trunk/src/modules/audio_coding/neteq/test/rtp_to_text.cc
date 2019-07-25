


















#include "data_log.h"
#include "NETEQTEST_DummyRTPpacket.h"
#include "NETEQTEST_RTPpacket.h"

#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <vector>





#define FIRSTLINELEN 40

using ::webrtc::DataLog;

int main(int argc, char* argv[])
{
    int arg_count = 1;
    NETEQTEST_RTPpacket* packet;

    if (argc < 3)
    {
      printf("Usage: %s [-d] <input_rtp_file> <output_base_name>\n", argv[0]);
      return -1;
    }

    
    if (argc >= 3 && strcmp(argv[arg_count], "-d") == 0)
    {
        packet = new NETEQTEST_DummyRTPpacket;
        ++arg_count;
    }
    else
    {
        packet = new NETEQTEST_RTPpacket;
    }

    std::string input_filename = argv[arg_count++];
    std::string table_name = argv[arg_count];

    std::cout << "Input file: " << input_filename << std::endl;
    std::cout << "Output file: " << table_name << ".txt" << std::endl;

    FILE *inFile=fopen(input_filename.c_str(),"rb");
    if (!inFile)
    {
        std::cout << "Cannot open input file " << input_filename << std::endl;
        return -1;
    }

    
    DataLog::CreateLog();
    if (DataLog::AddTable(table_name) < 0)
    {
        std::cout << "Error adding table " << table_name << ".txt" << std::endl;
        return -1;
    }

    DataLog::AddColumn(table_name, "seq", 1);
    DataLog::AddColumn(table_name, "ssrc", 1);
    DataLog::AddColumn(table_name, "payload type", 1);
    DataLog::AddColumn(table_name, "length", 1);
    DataLog::AddColumn(table_name, "timestamp", 1);
    DataLog::AddColumn(table_name, "marker bit", 1);
    DataLog::AddColumn(table_name, "arrival", 1);

    
    char firstline[FIRSTLINELEN];
    if (fgets(firstline, FIRSTLINELEN, inFile) == NULL)
    {
        std::cout << "Error reading file " << input_filename << std::endl;
        return -1;
    }

    
    if (fread(firstline, 4+4+4+2+2, 1, inFile) != 1)
    {
        std::cout << "Error reading file " << input_filename << std::endl;
        return -1;
    }

    while (packet->readFromFile(inFile) >= 0)
    {
        
        DataLog::InsertCell(table_name, "seq", packet->sequenceNumber());
        DataLog::InsertCell(table_name, "ssrc", packet->SSRC());
        DataLog::InsertCell(table_name, "payload type", packet->payloadType());
        DataLog::InsertCell(table_name, "length", packet->dataLen());
        DataLog::InsertCell(table_name, "timestamp", packet->timeStamp());
        DataLog::InsertCell(table_name, "marker bit", packet->markerBit());
        DataLog::InsertCell(table_name, "arrival", packet->time());
        DataLog::NextRow(table_name);
        return -1;
    }

    DataLog::ReturnLog();

    fclose(inFile);

    return 0;
}
