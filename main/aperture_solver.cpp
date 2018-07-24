/*
 * main.cpp
 *
 *  Created on: 
 *      Author: leslie
 */

#include <iostream>
#include <iterator>

#include "EvaluationFunction.h"
#include "Plan.h"
#include "Collimator.h"
#include "Volume.h"
#include "args.hxx"


using namespace std;
using namespace imrt;

double chooseBestClose(int beam, int a, Station& station, double c_eval, vector<double>& w, vector<double>& Zmin, vector<double>& Zmax, EvaluationFunction& F, Plan& P) {
  double aux_eval,dev=0;
  list<pair<int, double>> diff, aux_diff;
  bool best_left=false;
  
  aux_diff = station.closeBeamlet(beam, a, true);
  aux_eval = F.delta_eval(station, w, Zmin, Zmax, aux_diff);
  dev=F.eval(P,w,Zmin,Zmax);
  
  cout << "  Closing eval: " << aux_eval << " real: " << dev << endl; 
  if ( aux_eval <c_eval ) {
    best_left=true;
    c_eval = aux_eval;      
  } 
  station.undoLast();
  
  aux_diff = station.closeBeamlet(beam, a, false);
  aux_eval = F.delta_eval(station, w, Zmin, Zmax, aux_diff);
  dev=F.eval(P,w,Zmin,Zmax);  
  cout << "  Closing eval: " << aux_eval << " real: " << dev << endl; 
  
  if ( aux_eval >= c_eval ) {
    if (best_left) {
      aux_diff = station.closeBeamlet(beam, a, true);
    } else {
      station.undoLast();    
    }    
  } 
  return(c_eval);
}

double chooseBestOpen(int beam, int a, Station& station, double c_eval, vector<double>& w, vector<double>& Zmin, vector<double>& Zmax, EvaluationFunction& F, Plan& P) {
  double aux_eval=0, dev;
  list<pair<int, double>> aux_diff;

  aux_diff = station.openBeamlet(beam, a);
  aux_eval = F.delta_eval(station, w, Zmin, Zmax, aux_diff);
  dev=F.eval(P,w,Zmin,Zmax);
  
  cout << "  Opening eval: " << aux_eval << " real: " << dev << endl; 
  if ( aux_eval < c_eval ) {
    c_eval = aux_eval;      
  } else{
    station.undoLast();
  }

  return(c_eval);
}


double searchFirstAperture(int beam, Station& station, double c_eval, vector<double>& w, vector<double>& Zmin, vector<double>& Zmax, EvaluationFunction& F, bool close_beamlet, Plan & P) {
  double current=c_eval, aux_eval=0;
  list<pair<int, double>> diff, aux_diff;
  int a=0, last_a=station.getNbApertures()-1;
  bool flag=true;
  // Check every aperture 
  while(flag) {
    // Left op
    if (close_beamlet) {
      aux_eval=chooseBestClose(beam, a, station, c_eval, w, Zmin, Zmax, F, P); 
    } else {
      aux_eval=chooseBestOpen(beam, a, station, c_eval, w, Zmin, Zmax, F, P);
    }
    
    if (current> aux_eval) {
      current=aux_eval; 
      last_a=a;
    } 
        
    if (a==(station.getNbApertures()-1)) a=0;
    else a+=1;
    
    if (last_a==a) flag=false;
  }
  return(current);
}

double searchFirstIntensity(int beam, Station& station, double c_eval, vector<double>& w, vector<double>& Zmin, vector<double>& Zmax, EvaluationFunction& F, bool close_beamlet, Plan & P) {
  double current=c_eval, aux_eval=0;
  list<pair<int, double>> diff, aux_diff;
  int a=0, last_a=station.getNbApertures()-1;
  bool flag=true;
  // Check every aperture 
  while(flag) {
    // Left op
    if (close_beamlet) {
      station.modifyIntensityAperture(a, 1);
      aux_eval=F.eval(P,w,Zmin,Zmax); 
      cout << "  Increasing intensity aperture: " << a <<" eval: " << aux_eval<< endl;
    } else {
      station.modifyIntensityAperture(a, -1);
      aux_eval=F.eval(P,w,Zmin,Zmax); 
      cout << "  Reducing intensity aperture: " << a <<" eval: " << aux_eval<< endl;
    }
    
    if (current> aux_eval) {
      current=aux_eval; 
      last_a=a;
    } else {
      if (close_beamlet) 
        station.modifyIntensityAperture(a, -1);
      else
        station.modifyIntensityAperture(a, 1);
      
      if (a==(station.getNbApertures()-1)) a=0;
      else a+=1;
      if (last_a==a) flag=false;
    }
    
  }
  return(current);
}

int main(int argc, char** argv){

  int vsize=20;
  int bsize=5;
  double int0=4.0;
  int maxiter=10;
  int max_apertures=4;
  double alpha=1.0;
  double beta=1.0;
  double maxdelta=5.0;
  double maxratio=3.0;

	args::ArgumentParser parser("********* IMRT-Solver (Aperture solver) *********", "An IMRT Solver.");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	//args::ValueFlag<string> _format(parser, "string", "Format: (BR, BRw, 1C)", {'f'});
	args::ValueFlag<int> _bsize(parser, "int", "Number of considered beamlets for selection ("+to_string(bsize)+")", {"bsize"});
	args::ValueFlag<int> _vsize(parser, "int", "Number of considered worst voxels ("+to_string(vsize)+")", {"vsize"});
  args::ValueFlag<int> _int0(parser, "int", "Initial intensity for beams  ("+to_string(int0)+")", {"int0"});
  args::ValueFlag<int> _max_apertures(parser, "int", "Initial intensity for the station  ("+to_string(max_apertures)+")", {"max_ap"});
  args::ValueFlag<int> _maxdelta(parser, "int", "Max delta  ("+to_string(maxdelta)+")", {"maxdelta"});
  args::ValueFlag<int> _maxratio(parser, "int", "Max ratio  ("+to_string(maxratio)+")", {"maxratio"});
  args::ValueFlag<double> _alpha(parser, "double", "Initial temperature for intensities  ("+to_string(alpha)+")", {"alpha"});
  args::ValueFlag<double> _beta(parser, "double", "Initial temperature for ratio  ("+to_string(beta)+")", {"beta"});
  args::ValueFlag<int> _maxiter(parser, "int", "Number of iterations ("+to_string(maxiter)+")", {"max_iter"});
	//args::Flag trace(parser, "trace", "Trace", {"trace"});
	//args::Positional<std::string> _file(parser, "instance", "Instance");

	try
	{
		parser.ParseCLI(argc, argv);

	}
	catch (args::Help&)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}

  if(_bsize) bsize=_bsize.Get();
  if(_vsize) vsize=_vsize.Get();
  if(_maxdelta) maxdelta=_maxdelta.Get();
  if(_maxratio) maxratio=_maxratio.Get();
  if(_alpha) alpha=_alpha.Get();
  if(_beta) beta=_beta.Get();
  if(_int0) int0=_int0.Get();
  if(_maxiter) maxiter=_maxiter.Get();
  if(_max_apertures) max_apertures=_max_apertures.Get();

	vector< pair<int, string> > coord_files(5);
	coord_files[0]=(make_pair(0,"data/CERR_Prostate/CoordinatesBeam_0.txt"));
	coord_files[1]=(make_pair(70,"data/CERR_Prostate/CoordinatesBeam_70.txt"));
	coord_files[2]=(make_pair(140,"data/CERR_Prostate/CoordinatesBeam_140.txt"));
	coord_files[3]=(make_pair(210,"data/CERR_Prostate/CoordinatesBeam_210.txt"));
	coord_files[4]=(make_pair(280,"data/CERR_Prostate/CoordinatesBeam_280.txt"));

	vector<string> organ_files;
	organ_files.push_back("data/CERR_Prostate/DAO_DDM_BLADDER.dat");
	organ_files.push_back("data/CERR_Prostate/DAO_DDM_RECTUM.dat");
	organ_files.push_back("data/CERR_Prostate/DAO_DDM_PTVHD.dat");

  	Collimator collimator(coord_files);
  //	collimator.printAxisValues();
  //	collimator.printActiveBeam();

	vector<Volume> volumes;

  for (int i=0; i<organ_files.size(); i++) {
	  volumes.push_back(Volume(collimator, organ_files[i]));
	}

   //volumes[0].print_deposition();

   vector<Station*> stations(5);
   Station* station;
   for(int i=0;i<5;i++){
	   station = new Station(collimator,volumes, i*70, max_apertures);
	   station->generateIntensity();
	   stations[i]=station;
   }

   EvaluationFunction F(volumes);
   Plan P(F);
   for(int i=0;i<5;i++)
	   P.add_station(*stations[i]);


	vector<double> w={1,1,1};
	vector<double> Zmin={0,0,76};
	vector<double> Zmax={65,60,1000};

	double best_eval=F.eval(P,w,Zmin,Zmax);
	cout << "ev:" << best_eval << endl;
	
	//From here 
	auto sb=F.best_beamlets(P, bsize, vsize);
	auto it=sb.begin();
	Station*s = it->second.first; 
	int beamlet=it->second.second;
	bool sign=it->first.second;
	
	//Print initial intensity
	s->printIntensity();
	s->printAperture(0);
	
	/*
	
	list<pair<int,double>> aux;
	
	beamlet=57;
	aux = s->closeBeamlet(beamlet, 0, true);
	cout << "Diff" << endl;
	for (list<pair<int,double>>::iterator it=aux.begin();it!=aux.end();it++) {
	  cout << it->first << "," << it->second << endl;
	}
	s->printIntensity();
	s->printAperture(0);
	
	beamlet=53;
	aux = s->openBeamlet(beamlet, 0);
	cout << "Diff" << endl;
	for (list<pair<int,double>>::iterator it=aux.begin();it!=aux.end();it++) {
	  cout << it->first << "," << it->second << endl;
	}
	s->printIntensity();
	s->printAperture(0);*/
	
	

	for (int i=0;i<maxiter;i++) {
	  
		auto sb=F.best_beamlets(P, bsize, vsize);
		auto it=sb.begin();
		std::advance(it,rand()%sb.size());

		Station*s = it->second.first; 
		int beamlet=it->second.second;
		bool sign=it->first.second; //impact in F (+ or -)

    cout << "Iteration " << (i+1) << " current evaluation: " << best_eval << " evaluating beamlet: " << beamlet << " sign: " << sign << " station: " << s->getAngle() << endl;
    if (((double) rand() / (RAND_MAX)) < 0.5){
      best_eval=searchFirstIntensity(beamlet, *s, best_eval, w, Zmin, Zmax, F, !sign, P);
    } else {
	    best_eval=searchFirstAperture(beamlet, *s, best_eval, w, Zmin, Zmax, F, !sign, P);
    }
    
    s->printIntensity(false);

	}

	cout << endl;
	for(int i=0;i<5;i++){
		//stations[i]->printIntensity();
		stations[i]->printIntensity(true);
        //cout << "nb_apertures:" << stations[i]->int2nb.size() << endl;
    }
	cout << endl;

	cout << "best_eval:" << best_eval << endl;

	best_eval=F.eval(P,w,Zmin,Zmax);
	cout << "ev:" << best_eval << endl;

  F.generate_voxel_dose_functions ();
  system("python plotter/plot.py");

	return 0;

}
