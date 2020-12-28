#include "ClassLogFile.h"
#include "time_sntp.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "Helper.h"

static const char *TAG = "log";

ClassLogFile LogFile("/sdcard/log/message", "log_%Y-%m-%d.txt");

void ClassLogFile::WriteHeapInfo(std::string _id)
{
    std::string _zw;
    _zw = "\t" + _id + "\t" + getESPHeapInfo();
    WriteToFile(_zw);
}

/*
// code identical to Helper.cpp
// more detailed and structured info both for spi and internal heap
// SPI Heap:   larg.Blk:  min spi free   Internal heap 	  larg. Blk free  Min Heap free 

string getESPHeapInfo(){
	string espInfoResultStr = "";
	char aMsgBuf[80];
    
	multi_heap_info_t aMultiHead_info ;
	heap_caps_get_info (&aMultiHead_info,MALLOC_CAP_8BIT);
	size_t aFreeHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT);
	size_t aMinFreeHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
	size_t aHeapLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
	
	size_t aFreeSPIHeapSize  = heap_caps_get_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_SPIRAM);
	size_t aMinFreeSpiHeapSize =  heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_SPIRAM);
	size_t aSpiHeapLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT| MALLOC_CAP_SPIRAM);

 	size_t aFreeInternalHeapSize  = 		   heap_caps_get_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_INTERNAL);
	size_t aMinFreeInternalHeapSize =  		   heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT| MALLOC_CAP_INTERNAL);
	size_t aInternalHeapLargestFreeBlockSize = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT| MALLOC_CAP_INTERNAL);
	
	// sprintf(aMsgBuf," Heap: %ld", (long) aFreeHeapSize);
	//espInfoResultStr += string(aMsgBuf);
	//sprintf(aMsgBuf," Min Free: %ld", (long) aMinFreeHeapSize);
	//espInfoResultStr += string(aMsgBuf);
	//sprintf(aMsgBuf," larg. Block:  %ld", (long) aHeapLargestFreeBlockSize);
	//espInfoResultStr += string(aMsgBuf);
	
	sprintf(aMsgBuf," SPI Heap: %ld",        (long) aFreeSPIHeapSize);
	espInfoResultStr += string(aMsgBuf);
	sprintf(aMsgBuf," larg. Blk  free: %ld", (long) (aSpiHeapLargestFreeBlockSize));
	espInfoResultStr += string(aMsgBuf);
	sprintf(aMsgBuf," Min SPI free: %ld",    (long) (aMinFreeSpiHeapSize));
	espInfoResultStr += string(aMsgBuf);
	//sprintf(aMsgBuf," Min Free Heap Size: %ld", (long) aMinFreeHeapSize);
	//sprintf(aMsgBuf," NOT_SPI Heap: %ld", (long) (aFreeHeapSize - aFreeSPIHeapSize));
	//espInfoResultStr += string(aMsgBuf);
	//sprintf(aMsgBuf," largest Block Size:  %ld", (long) aHeapLargestFreeBlockSize);
	sprintf(aMsgBuf," Internal Heap: %ld",   (long) (aFreeInternalHeapSize));
	espInfoResultStr += string(aMsgBuf);
	sprintf(aMsgBuf," larg. Blk  free: %ld", (long) (aInternalHeapLargestFreeBlockSize));
	espInfoResultStr += string(aMsgBuf);
	sprintf(aMsgBuf," Min Heap free: %ld",   (long) (aMinFreeInternalHeapSize));
	espInfoResultStr += string(aMsgBuf);
	return 	espInfoResultStr;
}
*/

void ClassLogFile::WriteToDedicatedFile(std::string _fn, std::string info, bool _time)
{
    FILE* pFile;
    std::string zwtime;

    if (!doLogFile){
        return;
    }

//    pFile = OpenFileAndWait(_fn.c_str(), "a"); 
    pFile = fopen(_fn.c_str(), "a+");
    printf("Logfile opened: %s content: %s\n", _fn.c_str(),info.c_str());

    if (pFile!=NULL) {
        if (_time)
        {
            time_t rawtime;
            struct tm* timeinfo;
            char buffer[80];

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            strftime(buffer, 80, "%Y-%m-%d_%H-%M-%S", timeinfo);

            zwtime = std::string(buffer);
            info = zwtime + ": " + info;
        }
        fputs(info.c_str(), pFile);
        fputs("\n", pFile);

        fclose(pFile);    
    } else {
        ESP_LOGI(TAG, "Can't open log file %s", _fn.c_str());
    }
}

void ClassLogFile::SwitchOnOff(bool _doLogFile){
    doLogFile = _doLogFile;
};

void ClassLogFile::SetRetention(unsigned short _retentionInDays){
    retentionInDays = _retentionInDays;
};

void ClassLogFile::WriteToFile(std::string info, bool _time)
{
/*
    struct stat path_stat;
    if (stat(logroot.c_str(), &path_stat) != 0) {
        ESP_LOGI(TAG, "Create log folder: %s", logroot.c_str());
        if (mkdir_r(logroot.c_str(), S_IRWXU) == -1)  {
            ESP_LOGI(TAG, "Can't create log foolder");
        }
    }
*/
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[30];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 30, logfile.c_str(), timeinfo);
    std::string logpath = logroot + "/" + buffer; 
    
    WriteToDedicatedFile(logpath, info, _time);
}

std::string ClassLogFile::GetCurrentFileName()
{
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[60];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 60, logfile.c_str(), timeinfo);
    std::string logpath = logroot + "/" + buffer; 

    return logpath;
}

void ClassLogFile::RemoveOld()
{
    if (retentionInDays == 0) {
        return;
    }

    time_t rawtime;
    struct tm* timeinfo;
    char cmpfilename[30];

    time(&rawtime);
    rawtime = addDays(rawtime, -retentionInDays);
    timeinfo = localtime(&rawtime);
    
    strftime(cmpfilename, 30, logfile.c_str(), timeinfo);
    //ESP_LOGE(TAG, "log file name to compare: %s", cmpfilename);

    DIR *dir = opendir(logroot.c_str());
    if (!dir) {
        ESP_LOGI(TAG, "Failed to stat dir : %s", logroot.c_str());
        return;
    }

    struct dirent *entry;
    int deleted = 0;
    int notDeleted = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            //ESP_LOGI(TAG, "list log file : %s %s", entry->d_name, cmpfilename);
            if ((strlen(entry->d_name) == strlen(cmpfilename)) && (strcmp(entry->d_name, cmpfilename) < 0)) {
                ESP_LOGI(TAG, "delete log file : %s", entry->d_name);
                std::string filepath = logroot + "/" + entry->d_name; 
                if (unlink(filepath.c_str()) == 0) {
                    deleted ++;
                } else {
                    ESP_LOGE(TAG, "can't delete file : %s", entry->d_name);
                }
            } else {
                notDeleted ++;
            }
        }
    }
    ESP_LOGI(TAG, "%d older log files deleted. %d current log files not deleted.", deleted, notDeleted);
    closedir(dir);
}

ClassLogFile::ClassLogFile(std::string _logroot, std::string _logfile)
{
    logroot = _logroot;
    logfile =  _logfile;
    doLogFile = true;
    retentionInDays = 10;
}
