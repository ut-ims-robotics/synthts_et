class CDisambiguator;
class CLinguistic;
extern "C" {
#include "HTS_engine.h"
} 

class EstonianSpeechSynthesis {
    CDisambiguator* Disambiguator = 0;
    CLinguistic* Linguistic = 0;
    HTS_Engine* engine = 0;
    char **fn_voices;
    double speed = 1.1;
    size_t fr = 48000;
    size_t fp = 240;
    float alpha = 0.55;
    float beta = 0.0;
    float ht = 2.0;
    float th = 0.5;
    float gvw1 = 1.0;
    float gvw2 = 1.2;
    bool write_raw = false;

public:
    EstonianSpeechSynthesis ();
    bool initialize();
    bool destroy();
    size_t convertTextToWave (char* textToSpeak, const char* wavfilename);
};


