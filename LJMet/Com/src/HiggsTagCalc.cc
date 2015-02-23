/*
  Adapted from: singleLepCalc.cc and JetSubCalc.cc
  Author: Rizki Syarif, November 2014
*/

#include <iostream>
#include "LJMet/Com/interface/BaseCalc.h"
#include "LJMet/Com/interface/LjmetFactory.h"
#include "LJMet/Com/interface/LjmetEventContent.h"
#include "TLorentzVector.h"
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include "DataFormats/Math/interface/deltaR.h"

#include <fastjet/JetDefinition.hh>
#include <fastjet/PseudoJet.hh>
#include <fastjet/ClusterSequenceArea.hh>
#include "DataFormats/PatCandidates/interface/Jet.h"

//add SD includes - start
#include "ShowerDeconstruction/test/src/Exception.h"
#include "ShowerDeconstruction/test/src/AnalysisParameters.h"
#include "ShowerDeconstruction/test/src/HBBModel.h"
#include "ShowerDeconstruction/test/src/BackgroundModel.h"
#include "ShowerDeconstruction/test/src/ISRModel.h"
#include "ShowerDeconstruction/test/src/Deconstruct.h"
#include "ShowerDeconstruction/test/src/Message.h"
//add SD includes - end

using namespace Deconstruction;
using namespace fastjet;

class LjmetFactory;

class HiggsTagCalc : public BaseCalc{
  
public:
  
  HiggsTagCalc();
  virtual ~HiggsTagCalc(){}

  virtual int BeginJob(){

    if (mPset.exists("CA8JetColl")) CA8JetColl_it = mPset.getParameter<edm::InputTag>("CA8JetColl");
    else                            CA8JetColl_it = edm::InputTag("goodPatJetsCA8PF");
    
    if (mPset.exists("input_card")) input_card_ = mPset.getParameter<std::string>("input_card");
    else                            input_card_ = "/home/rsyarif/LJMet/CMSSW_7_0_7/src/ShowerDeconstruction/input_card.dat";

    if (mPset.exists("microjet_conesize")) microjet_conesize_ = mPset.getParameter<double>("microjet_conesize");
    else                            microjet_conesize_ = 0.2;
    
    return 0;
  }

  virtual int AnalyzeEvent(edm::EventBase const & event, BaseEventSelector * selector);
  virtual int EndJob(){return 0;}

  
private:
  edm::InputTag CA8JetColl_it;   
  std::string input_card_;   
  double microjet_conesize_;   
};


static int reg = LjmetFactory::GetInstance()->Register(new HiggsTagCalc(), "HiggsTagCalc");


HiggsTagCalc::HiggsTagCalc(){
}

int HiggsTagCalc::AnalyzeEvent(edm::EventBase const & event,
				BaseEventSelector * selector){
  //
  // compute event variables here
  //

  //four vector
  std::vector <double> CA8JetPt;
  std::vector <double> CA8JetEta;
  std::vector <double> CA8JetPhi;
  std::vector <double> CA8JetEnergy;
  std::vector <double> CA8JetMass;

  //Get all CA8 jets (not just for W and Top)
  edm::Handle<std::vector<pat::Jet> > CA8Jets;
  event.getByLabel(CA8JetColl_it, CA8Jets);

  std::vector<double> PSig;
  std::vector<double> PBkg;
  std::vector<double> chi;

  for (std::vector<pat::Jet>::const_iterator ijet = CA8Jets->begin(); ijet != CA8Jets->end(); ijet++){

    //Four vector
    CA8JetPt     . push_back(ijet->pt());
    CA8JetEta    . push_back(ijet->eta());
    CA8JetPhi    . push_back(ijet->phi());
    CA8JetEnergy . push_back(ijet->energy());
    CA8JetMass   . push_back(ijet->mass());

    //Make FastJet constituents
    std::vector<fastjet::PseudoJet> FJConstituents;
    for (unsigned i = 0; i < ijet->numberOfDaughters() ; i++){
      const reco::PFCandidate* this_constituent = dynamic_cast<const reco::PFCandidate*>(ijet->daughter(i));
      FJConstituents.push_back( fastjet::PseudoJet( this_constituent->px(),
						 this_constituent->py(),
						 this_constituent->pz(),
						 this_constituent->energy() ) );
    }

    //Shower deconstruction - start 

    //define "microjets" (clustering algo, R parameter)
    JetDefinition microjet_def(fastjet::kt_algorithm, microjet_conesize_);
    ClusterSequence clust_seq_microjet(FJConstituents, microjet_def);
    vector<PseudoJet> microjets = sorted_by_pt(clust_seq_microjet.inclusive_jets(20));

    //remove the low pt micro jets if there are more than 7 microjets                                                                                        
    if (microjets.size() > 7) {
      microjets.erase(microjets.begin() + (int) 7,
                      microjets.begin() + microjets.size());
    }
    
    //std::string inputcard = "/home/rsyarif/LJMet/CMSSW_7_0_7/src/input_card.dat"; //load the input parameters file. where to put?        

    
    AnalysisParameters param(input_card_);                                                                                
    HBBModel *signal = 0;                                                                                           
    BackgroundModel *background = 0;                                                                            
    ISRModel *isr = 0;     
    Deconstruct *deconstruct = 0;                                                                                    
    
    signal = new HBBModel(param);
    background = new BackgroundModel(param);
    isr = new ISRModel(param);
    deconstruct = new Deconstruct(param, *signal, *background, *isr);

    double Psignal = 0.0;
    double Pbackground = 0.0;
    double chi_;
    try {
      LOGLEVEL(INFO); //debugging - change DEBUG to INFO to turn off (not sure if it is intended this way).
      chi_ = deconstruct->deconstruct(microjets, Psignal, Pbackground); //call SD                                                                                
    }
    catch(Deconstruction::Exception &e) {
      std::cout << "Exception while running SD: " << e.what() << std::endl;
    }
    //Shower Deconstruction - end       

    PSig.push_back(Psignal);
    PBkg.push_back(Pbackground);
    chi.push_back(chi_);
    std::cout << "Psig  = " << Psignal << std::endl; //rizki 
    std::cout << "PBkg = " << Pbackground << std::endl; //rizki 
    std::cout << "chi = " << chi_ << std::endl; //rizki
    std::cout << std::endl;
  }
  
  SetValue("PSignal", PSig);
  SetValue("PBackground", PBkg);
  SetValue("chi", chi);

  return 0;
}
