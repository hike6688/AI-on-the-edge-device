#include "ClassFlowPostProcessing.h"

#include "Helper.h"
#include "ClassFlowAnalog.h"
#include "ClassFlowDigit.h"
#include "ClassFlowMakeImage.h"
#include "ClassLogFile.h"

#include <iomanip>
#include <sstream>

#include <time.h>

string ClassFlowPostProcessing::GetPreValue()
{
    std::string result;
    result = RundeOutput(PreValue, -DecimalShift);

    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowAnalog") == 0)
        {
            int AnzahlAnalog = ((ClassFlowAnalog*)(*ListFlowControll)[i])->AnzahlROIs();
            result =  RundeOutput(PreValue, AnzahlAnalog - DecimalShift);
        }
    }

    return result;
}

bool ClassFlowPostProcessing::LoadPreValue(void)
{
    FILE* pFile;
    char zw[1024];
    string zwtime, zwvalue;

    pFile = fopen(FilePreValue.c_str(), "r");
    if (pFile == NULL)
        return false;

    fgets(zw, 1024, pFile);
    printf("%s", zw);
    zwtime = trim(std::string(zw));

    fgets(zw, 1024, pFile);
    fclose(pFile);
    printf("%s", zw);
    zwvalue = trim(std::string(zw));
    PreValue = stof(zwvalue.c_str());

    time_t tStart;
    int yy, month, dd, hh, mm, ss;
    struct tm whenStart;

    sscanf(zwtime.c_str(), "%d-%d-%d_%d-%d-%d", &yy, &month, &dd, &hh, &mm, &ss);
    whenStart.tm_year = yy - 1900;
    whenStart.tm_mon = month - 1;
    whenStart.tm_mday = dd;
    whenStart.tm_hour = hh;
    whenStart.tm_min = mm;
    whenStart.tm_sec = ss;
    whenStart.tm_isdst = -1;

    tStart = mktime(&whenStart);

    time_t now;
    time(&now);
    localtime(&now);
    double difference = difftime(now, tStart);
    difference /= 60;
    if (difference > PreValueAgeStartup)
        return false;

    Value = PreValue;
    ReturnValue = to_string(Value);
    ReturnValueNoError = ReturnValue; 

    // falls es Analog gibt, dann die Anzahl der Nachkommastellen feststellen und entsprechend runden:   
    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowAnalog") == 0)
        {
            int AnzahlAnalog = ((ClassFlowAnalog*)(*ListFlowControll)[i])->AnzahlROIs();
            ReturnValue = RundeOutput(Value, AnzahlAnalog - DecimalShift);
            ReturnValueNoError = ReturnValue;
        }
    }
    
    return true;
}

void ClassFlowPostProcessing::SavePreValue(float value, string zwtime)
{
    FILE* pFile;

    pFile = fopen(FilePreValue.c_str(), "w");

    if (strlen(zwtime.c_str()) == 0)
    {
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, 80, "%Y-%m-%d_%H-%M-%S", timeinfo);
        zwtime = std::string(buffer);
    }

    fputs(zwtime.c_str(), pFile);
    fputs("\n", pFile);
    fputs(to_string(value).c_str(), pFile);
    fputs("\n", pFile);

    fclose(pFile);
}



ClassFlowPostProcessing::ClassFlowPostProcessing()
{
    PreValueUse = false;
    PreValueAgeStartup = 30;
    AllowNegativeRates = false;
    MaxRateValue = 0.1;
    ErrorMessage = false;
    ListFlowControll = NULL;
    PreValueOkay = false;
    useMaxRateValue = false;
    checkDigitIncreaseConsistency = false;
    DecimalShift = 0;
    ErrorMessageText = "";
    FilePreValue = FormatFileName("/sdcard/config/prevalue.ini");
}

ClassFlowPostProcessing::ClassFlowPostProcessing(std::vector<ClassFlow*>* lfc)
{
    PreValueUse = false;
    PreValueAgeStartup = 30;
    AllowNegativeRates = false;
    MaxRateValue = 0.1;
    ErrorMessage = false;
    ListFlowControll = NULL;
    PreValueOkay = false;
    useMaxRateValue = false;
    checkDigitIncreaseConsistency = false;
    DecimalShift = 0;    
    ErrorMessageText = "";
    FilePreValue = FormatFileName("/sdcard/config/prevalue.ini");
    ListFlowControll = lfc;
}


bool ClassFlowPostProcessing::ReadParameter(FILE* pfile, string& aktparamgraph)
{
    std::vector<string> zerlegt;

    aktparamgraph = trim(aktparamgraph);

    if (aktparamgraph.size() == 0)
        if (!this->GetNextParagraph(pfile, aktparamgraph))
            return false;


    if (aktparamgraph.compare("[PostProcessing]") != 0)       // Paragraph passt nich zu MakeImage
        return false;

    while (this->getNextLine(pfile, &aktparamgraph) && !this->isNewParagraph(aktparamgraph))
    {
        zerlegt = this->ZerlegeZeile(aktparamgraph);
        if ((toUpper(zerlegt[0]) == "DECIMALSHIFT") && (zerlegt.size() > 1))
        {
            DecimalShift = stoi(zerlegt[1]);
        }

        if ((toUpper(zerlegt[0]) == "PREVALUEUSE") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
            {
                PreValueUse = true;
            }
        }
        if ((toUpper(zerlegt[0]) == "CHECKDIGITINCREASECONSISTENCY") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                checkDigitIncreaseConsistency = true;
        }        
        if ((toUpper(zerlegt[0]) == "ALLOWNEGATIVERATES") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                AllowNegativeRates = true;
        }
        if ((toUpper(zerlegt[0]) == "ERRORMESSAGE") && (zerlegt.size() > 1))
        {
            if (toUpper(zerlegt[1]) == "TRUE")
                ErrorMessage = true;
        }
        if ((toUpper(zerlegt[0]) == "PREVALUEAGESTARTUP") && (zerlegt.size() > 1))
        {
            PreValueAgeStartup = std::stoi(zerlegt[1]);
        }
        if ((toUpper(zerlegt[0]) == "MAXRATEVALUE") && (zerlegt.size() > 1))
        {
            useMaxRateValue = true;
            MaxRateValue = std::stof(zerlegt[1]);
        }
    }

    if (PreValueUse) {
        PreValueOkay = LoadPreValue();
    }
    return true;
}

string ClassFlowPostProcessing::ShiftDecimal(string in, int _decShift){

    if (_decShift == 0){
        return in;
    }

    int _pos_dec_org, _pos_dec_neu;

    _pos_dec_org = findDelimiterPos(in, ".");
    if (_pos_dec_org == std::string::npos) {
        _pos_dec_org = in.length();
    }
    else
    {
        in = in.erase(_pos_dec_org, 1);
    }
    
    _pos_dec_neu = _pos_dec_org + _decShift;

    if (_pos_dec_neu <= 0) {        // Komma ist vor der ersten Ziffer
        for (int i = 0; i > _pos_dec_neu; --i){
            in = in.insert(0, "0");
        }
        in = "0." + in;
        return in;
    }

    if (_pos_dec_neu > in.length()){    // Komma soll hinter String (123 --> 1230)
        for (int i = in.length(); i < _pos_dec_neu; ++i){
            in = in.insert(in.length(), "0");
        }  
        return in;      
    }

    string zw;
    zw = in.substr(0, _pos_dec_neu);
    zw = zw + ".";
    zw = zw + in.substr(_pos_dec_neu, in.length() - _pos_dec_neu);

    return zw;
}

bool ClassFlowPostProcessing::doFlow(string zwtime)
{
    std::string aFktId ="ClassFlowPostProcessing::doFlow ";
    string result = "";
    string digit = "";
    string analog = "";
    string zwvalue;
    bool isdigit = false;
    bool isanalog = false;
    int AnzahlAnalog = 0;
    string zw;
    time_t imagetime = 0;

    ErrorMessageText = "";


    for (int i = 0; i < ListFlowControll->size(); ++i)
    {
        if (((*ListFlowControll)[i])->name().compare("ClassFlowMakeImage") == 0)
        {
            imagetime = ((ClassFlowMakeImage*)(*ListFlowControll)[i])->getTimeImageTaken();
        }
        if (((*ListFlowControll)[i])->name().compare("ClassFlowDigit") == 0)
        {
            isdigit = true;
            digit = (*ListFlowControll)[i]->getReadout();
        }
        if (((*ListFlowControll)[i])->name().compare("ClassFlowAnalog") == 0)
        {
            isanalog = true;
            analog = (*ListFlowControll)[i]->getReadout();
            AnzahlAnalog = ((ClassFlowAnalog*)(*ListFlowControll)[i])->AnzahlROIs();
        }
    }

    if (imagetime == 0)
        time(&imagetime);

    struct tm* timeinfo;
    timeinfo = localtime(&imagetime);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d_%H-%M-%S", timeinfo);
    zwtime = std::string(strftime_buf);


    //    // TESTING ONLY////////////////////
    //    isdigit = true; digit = "12N";
    //    isanalog = true; analog = "456";

    if (isdigit)
        ReturnRawValue = digit;
    if (isdigit && isanalog)
        ReturnRawValue = ReturnRawValue + ".";
    if (isanalog)
        ReturnRawValue = ReturnRawValue + analog; 

    LogFile.WriteToFile(aFktId + " ReturnRawValue: " + ReturnRawValue);

    ReturnRawValue = ShiftDecimal(ReturnRawValue, DecimalShift);   
    if (!PreValueUse || !PreValueOkay)
    {
        ReturnValue = ReturnRawValue;
        ReturnValueNoError = ReturnRawValue;

        if ((findDelimiterPos(ReturnValue, "N") == std::string::npos) && (ReturnValue.length() > 0))
        {
            while ((ReturnValue.length() > 1) && (ReturnValue[0] == '0'))
            {
                ReturnValue.erase(0, 1);
            }
            Value = std::stof(ReturnValue);
            ReturnValueNoError = ReturnValue;

            PreValueOkay = true;
            PreValue = Value;
            
            SavePreValue(Value, zwtime);
        }

        LogFile.WriteToFile(aFktId + " ReturnValue: " + ReturnValue);

        return true;
    }

    zw = ErsetzteN(ReturnRawValue); 

    Value = std::stof(zw);
    if (checkDigitIncreaseConsistency)
    {
        Value = checkDigitConsistency(Value, DecimalShift, isanalog);
    }

    zwvalue = RundeOutput(Value, AnzahlAnalog - DecimalShift);

    if ((!AllowNegativeRates) && (Value < PreValue))
    {
        ErrorMessageText = ErrorMessageText + "Negative Rate - Returned old value - read value: " + zwvalue + " ";
        Value = PreValue;
        zwvalue = RundeOutput(Value, AnzahlAnalog - DecimalShift);
    }

    if (useMaxRateValue && (abs(Value - PreValue) > MaxRateValue))
    {
        ErrorMessageText = ErrorMessageText + "Rate too high - Returned old value - read value: " + zwvalue + " ";
        Value = PreValue;
        zwvalue = RundeOutput(Value, AnzahlAnalog - DecimalShift);
    }

    ReturnValueNoError = zwvalue;
    ReturnValue = zwvalue;
    if (ErrorMessage && (ErrorMessageText.length() > 0))
        ReturnValue = ReturnValue + "\t" + ErrorMessageText;

    if (ErrorMessageText.length() == 0)
    {
        PreValue = Value;
        
        SavePreValue(Value, zwtime);
       
    }

    LogFile.WriteToFile(aFktId + " ReturnValue: " + ReturnValue);
    
    return true;
}

string ClassFlowPostProcessing::getReadout()
{
    return ReturnValue;
}

string ClassFlowPostProcessing::getReadoutParam(bool _rawValue, bool _noerror)
{
    if (_rawValue)
        return ReturnRawValue;
    if (_noerror)
        return ReturnValueNoError;
    return ReturnValue;
}

string ClassFlowPostProcessing::RundeOutput(float _in, int _anzNachkomma){
    std::stringstream stream;
    if (_anzNachkomma < 0) {
        _anzNachkomma = 0;
    }
    stream << std::fixed << std::setprecision(_anzNachkomma) << _in;
    return stream.str();  
}


string ClassFlowPostProcessing::ErsetzteN(string input)
{
    int posN, posPunkt;
    int pot, ziffer;
    float zw;

    posN = findDelimiterPos(input, "N");
    posPunkt = findDelimiterPos(input, ".");
    if (posPunkt == std::string::npos){
        posPunkt = input.length();
    }

    while (posN != std::string::npos)
    {
        if (posN < posPunkt) {
            pot = posPunkt - posN - 1;
        }
        else {
            pot = posPunkt - posN;
        }

        zw = PreValue / pow(10, pot);
        ziffer = ((int) zw) % 10;
        input[posN] = ziffer + 48;

        posN = findDelimiterPos(input, "N");
    }

    return input;
}

float ClassFlowPostProcessing::checkDigitConsistency(float input, int _decilamshift, bool _isanalog){
    int aktdigit, olddigit;
    int aktdigit_before, olddigit_before;
    int pot, pot_max;
    float zw;
    bool no_nulldurchgang = false;

    pot = _decilamshift;
    if (!_isanalog)             // falls es keine analogwerte gibt, kann die letzte nicht bewertet werden
    {
        pot++;
    }
    pot_max = ((int) log10(input)) + 1;

    while (pot <= pot_max)
    {
        zw = input / pow(10, pot-1);
        aktdigit_before = ((int) zw) % 10;
        zw = PreValue / pow(10, pot-1);
        olddigit_before = ((int) zw) % 10;

        zw = input / pow(10, pot);
        aktdigit = ((int) zw) % 10;
        zw = PreValue / pow(10, pot);
        olddigit = ((int) zw) % 10;

        no_nulldurchgang = (olddigit_before <= aktdigit_before);

        if (no_nulldurchgang)
        {
            if (aktdigit != olddigit) 
            {
                input = input + ((float) (olddigit - aktdigit)) * pow(10, pot);     // Neue Digit wird durch alte Digit ersetzt;
            }
        }
        else
        {
            if (aktdigit == olddigit)                   // trotz Nulldurchgang wurde Stelle nicht hochgezählt --> addiere 1
            {
                input = input + ((float) (1)) * pow(10, pot);   // addiere 1 an der Stelle
            }
        }

        pot++;
    }

    return input;
}


string ClassFlowPostProcessing::getReadoutError() 
{
    return ErrorMessageText;
}
