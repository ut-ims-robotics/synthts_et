#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "../lib/etana/proof.h"
#include "../include/mklab.h"
extern "C" {
#include "../include/HTS_engine.h"
}
#include "estsynth.h"

typedef unsigned char uchar;

wchar_t *UTF8_to_WChar(const char *string) {
    long b = 0,
            c = 0; 
    for (const char *a = string; *a; a++)
        if (((uchar) * a) < 128 || (*a & 192) == 192)
            c++;
    wchar_t *res = new wchar_t[c + 1];

    res[c] = 0;
    for (uchar *a = (uchar*) string; *a; a++) {
        if (!(*a & 128))
            res[b] = *a;
        else if ((*a & 192) == 128)
            continue;
        else if ((*a & 224) == 192)
            res[b] = ((*a & 31) << 6) | a[1]&63;
        else if ((*a & 240) == 224)
            res[b] = ((*a & 15) << 12) | ((a[1]&63) << 6) | a[2]&63;
        else if ((*a & 248) == 240) {
            res[b] = '?';
        }
        b++;
    }
    return res;
}

void ReadUTF8Text(CFSWString &text, const char *fn) {

    std::ifstream fs;
    fs.open(fn, std::ios::binary);
    if (fs.fail()) {
        fprintf(stderr,"Ei leia sisendteksti!\n");
        exit(1);
    }
    fs.seekg(0, std::ios::end);
    size_t i = fs.tellg();    
    fs.seekg(0, std::ios::beg);
    char* buf = new char[i+1];
    fs.read(buf, i);
    fs.close();
    buf[i] = '\0';
    wchar_t* w_temp = UTF8_to_WChar(buf);
    text = w_temp;
    delete [] buf;
    delete [] w_temp;
}

char *convert_vec(const std::string & s) {
    char *pc = new char[s.size() + 1];
    strcpy(pc, s.c_str());
    return pc;
}

void fill_char_vector(std::vector<std::string>& v, std::vector<char*>& vc) {
    std::transform(v.begin(), v.end(), std::back_inserter(vc), convert_vec);
}

void clean_char_vector(std::vector<char*>& vc) {
    for (size_t x = 0; x < vc.size(); x++)
        delete [] vc[x];
}

std::string to_stdstring(CFSWString s) {
    std::string res = "";
    for (INTPTR i = 0; i < s.GetLength(); i++)
        res += s.GetAt(i);
    return res;
}

std::vector<std::string> to_vector(CFSArray<CFSWString> arr) {
    std::vector<std::string> v;
    for (INTPTR i = 0; i < arr.GetSize(); i++)
        v.push_back(to_stdstring(arr[i]));
    return v;
}



    EstonianSpeechSynthesis::EstonianSpeechSynthesis () {
        fn_voices = 0;
    }

    bool EstonianSpeechSynthesis::initialize () {
 
        CFSAString LexFileName ("dct/et.dct");
        CFSAString LexDFileName ("dct/et3.dct");
        std::string voiceFileName ("htsvoices/eki_et_tnu.htsvoice"); 

        Disambiguator = new CDisambiguator();
        Linguistic = new CLinguistic();
        engine = new HTS_Engine ();

        FSCInit();
        fn_voices = (char **) malloc(sizeof (char *));
        fn_voices[0] = strdup(voiceFileName.c_str());

        Linguistic->Open(LexFileName);
        Disambiguator->Open(LexDFileName);
        HTS_Engine_initialize(engine);

        if (HTS_Engine_load(engine, fn_voices, 1) != TRUE) {
            fprintf(stderr, "Viga: puudub *.htsvoice. %p\n", fn_voices[0]);
            free(fn_voices);
            HTS_Engine_clear(engine);
            return false;
        }
        free(fn_voices);

        HTS_Engine_set_sampling_frequency(engine, (size_t) fr);
        HTS_Engine_set_phoneme_alignment_flag(engine, FALSE);
        HTS_Engine_set_fperiod(engine, (size_t) fp);
        HTS_Engine_set_alpha(engine, alpha);
        HTS_Engine_set_beta(engine, beta);
        HTS_Engine_set_speed(engine, speed);
        HTS_Engine_add_half_tone(engine, ht);
        HTS_Engine_set_msd_threshold(engine, 1, th);
        HTS_Engine_set_gv_weight(engine, 0, gvw1);
        HTS_Engine_set_gv_weight(engine, 1, gvw2);
    }

    bool EstonianSpeechSynthesis::destroy() {
        HTS_Engine_clear(engine);
        Linguistic->Close();
        FSCTerminate();
    }

    size_t EstonianSpeechSynthesis::convertTextToWave (char* textToSpeak, const char* wavfilename) {
 
        // Text to be converted
        CFSWString text (UTF8_to_WChar(textToSpeak));
        text = DealWithText(text);

        // File to be written to
        FILE *file = 0;
        if (file = fopen(wavfilename, "r")) {
            fclose(file);
            remove(wavfilename);
        }
    
        CFSArray<CFSWString> res = do_utterances(text);

        INTPTR data_size = 0;

        FILE *outfp = fopen(wavfilename, "wb");

        if (!write_raw)
            HTS_Engine_write_header(engine, outfp, 1);
        for (INTPTR i = 0; i < res.GetSize(); i++) {

            CFSArray<CFSWString> label = do_all(res[i], false, false, *Disambiguator, *Linguistic);

            std::vector<std::string> v;
            v = to_vector(label);

            std::vector<char*> vc;
            fill_char_vector(v, vc);

            size_t n_lines = vc.size();

            if (HTS_Engine_synthesize_from_strings(engine, &vc[0], n_lines) != TRUE) {
                fprintf(stderr, "Viga: süntees ebaonnestus.\n");            
                HTS_Engine_clear(engine);
                return false;
            }

            clean_char_vector(vc);
            data_size += HTS_Engine_engine_speech_size(engine);


 
            HTS_Engine_save_generated_speech(engine, outfp);
            HTS_Engine_refresh(engine);
        } //synth loop

        if (!write_raw)
            HTS_Engine_write_header(engine, outfp, data_size);

        fclose(outfp);

        float numSeconds = ceil((double)data_size / (double)sizeof(short) / (double)fr);
        return size_t(numSeconds);
    }

/*
int main(int argc, char* argv[]) {

    EstonianSpeechSynthesis ess;
    ess.initialize();
    if (!ess.convertTextToWave (argv[1], argv[2])) {
        fprintf (stderr, "No conversion today, sry.\n");
    }
    fprintf (stderr, "1,2,3, techno.\n");
    return 0;

}
*/

