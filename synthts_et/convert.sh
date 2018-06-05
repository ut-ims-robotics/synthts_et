#!/bin/sh
cd /home/jan/synthts_et-master/synthts_et
bin/synthts_et -lex dct/et.dct -lexd dct/et3.dct -o out_tnu.wav -f in.txt -m htsvoices/eki_et_tnu.htsvoice -r 1.1
