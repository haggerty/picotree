#include <iostream>
#include <fstream>
#include <sstream>

#include "TString.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TFile.h"
#include "TTree.h"
#include "TSystem.h"

// Program to take the saved waveform csv files created by the Picoscope software and make a root file of them
// Each trigger is in its own csv file, so this loops over all the csv files in the directory created by the Picoscope software
// John Haggerty, BNL, 2019.02.07

const Int_t headerlines = 3;

struct onecsv {
    Int_t nchannels;
    Int_t nsamples;
    std::vector<double> t;
    std::vector<double> cha;
    std::vector<double> chb;
    std::vector<double> chc;
    std::vector<double> chd;
};

onecsv csvtranslator( TString filename )
{
    onecsv currentfile;

    std::ifstream ifs( filename );
    std::string firstline;
    std::string secondline;
    std::string thirdline;
    std::string line;

    getline(ifs,firstline);
    //  std::cout << firstline << std::endl;

    // firstline looks like this: Time,Channel A,Channel B,Channel C
    const Int_t nchan = count(firstline.begin(), firstline.end(), ',');

    // secondline looks like this: (ns),(V),(mV),(mV)
    getline(ifs,secondline);
    //  std::cout << secondline << std::endl;

    // thirdline is blank
    getline(ifs,thirdline);
    //  std::cout << thirdline << std::endl;

    // data lines look like this: 94.22221940,-1.00855100,15.90182000,-3.98314500
    // a pathology looks like this: -17.91999943,-0.01574803,-Infinity,-302.93430000
    Double_t t;
    Double_t adc[nchan];
    string token;
    Int_t nsam = 0;
    while ( getline(ifs,line) ) {
        Int_t pos = 0;
        std::istringstream iss(line);
        while ( getline(iss, token, ',')) {
            //    std::cout << token << std::endl;
            //    std::cout << pos << std::endl;
            if ( pos == 0 ) {
                currentfile.t.push_back( atof(token.c_str()) );
                //	t =  atof(token.c_str());
                //	std::cout << t << ": ";
                nsam++;
            } else if ( pos == 1 ) {
                currentfile.cha.push_back( ( token.find("Infinity") != std::string::npos ) ? std::numeric_limits<double>::quiet_NaN() : atof(token.c_str()) );	
            } else if ( pos == 2 ) {
                currentfile.chb.push_back( ( token.find("Infinity") != std::string::npos ) ? std::numeric_limits<double>::quiet_NaN() : atof(token.c_str()) );	
            } else if ( pos == 3 ) {
                currentfile.chc.push_back( ( token.find("Infinity") != std::string::npos ) ? std::numeric_limits<double>::quiet_NaN() : atof(token.c_str()) );	
            } else if ( pos == 4 ) {
                currentfile.chd.push_back( ( token.find("Infinity") != std::string::npos ) ? std::numeric_limits<double>::quiet_NaN() : atof(token.c_str()) );	
            }
            if ( pos == nchan ) {
                // if you get to nchan, break out of the loop because there are more channels than expected
                //	std::cout << std::endl;
                break;
            }
            pos++;
        }
    }

    currentfile.nchannels = nchan;
    currentfile.nsamples = nsam;

    ifs.close();  

    return currentfile;
}

Int_t picocsv2ttree( TString directory )
{
    onecsv currentfile;

    // remove / if it's at the end of the directory name 
    if ( directory.EndsWith("/") ) directory.Chop();
    std::cout << "csv files in " << directory << std::endl;
    TSystem *sys = new TSystem();
    TString rootfilename = directory + ".root";
    // this will make the root file in the pwd
    //  rootfilename =  sys->BaseName( directory + ".root" );
    std::cout << "creating " << rootfilename << std::endl;
    TFile *fr = new TFile(rootfilename,"recreate");
    TTree *ps = new TTree("ps","Picoscope 3406B data");

    // the following will loop over all the data files in numerical order
    Int_t nfiles = 0;
    Int_t fileno = 0;
    TString ext = ".csv";
    TSystemDirectory dir(directory, directory); 
    TList *files = dir.GetListOfFiles(); 
    if (files) { 
        files->Sort();
        TSystemFile *file; 
        TString fname; 
        TIter next(files); 
        TString fullfilename;
        Bool_t firstfile = true;
        Int_t nchannels_first = 0;
        Int_t nsamples_first = 0;
        while ( (file=(TSystemFile*)next()) ) {
            fname = file->GetName(); 
            if (!file->IsDirectory() && fname.EndsWith(ext)) { 

                // extract the file number from fname with string gyrations
                std::string sfilename = fname.Data();
                Int_t first = sfilename.find_last_of("_");
                Int_t last = sfilename.find_last_of(".");
                fileno = atoi( sfilename.substr( first + 1, last - first -1 ).c_str() );

                std::cout << fname.Data() << " " << fileno << std::endl; 

                fullfilename = directory + "/" + fname;
                currentfile  = csvtranslator(fullfilename);
                //	std::cout << "channels: " << currentfile.nchannels << " samples: " << currentfile.nsamples << std::endl;

                if ( firstfile ) {
                    std::cout << "adding branches" << std::endl;
                    ps->Branch("file",&nfiles,"file/I");
                    ps->Branch("t",&currentfile.t);
                    if ( currentfile.nchannels >= 1 ) ps->Branch("cha",&currentfile.cha);
                    if ( currentfile.nchannels >= 2 ) ps->Branch("chb",&currentfile.chb);
                    if ( currentfile.nchannels >= 3 ) ps->Branch("chc",&currentfile.chc);
                    if ( currentfile.nchannels >= 4 ) ps->Branch("chd",&currentfile.chd);

                    nchannels_first = currentfile.nchannels;
                    nsamples_first = currentfile.nsamples;
                    std::cout << "Channels: " << nchannels_first << " Samples: " << nsamples_first << std::endl;
                    firstfile = false;
                }
                if ( currentfile.nchannels != nchannels_first || currentfile.nsamples != nsamples_first ) {
                    std::cout << "uh-oh! channels: " << currentfile.nchannels << " samples: " << currentfile.nsamples << std::endl;
                }
                ps->Fill();
                nfiles++;
            }
        }
        ps->Write();
        fr->Close();
    } 

    return nfiles;
}
