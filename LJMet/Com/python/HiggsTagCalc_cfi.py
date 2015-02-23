import FWCore.ParameterSet.Config as cms

HiggsTagCalc = cms.PSet(
    CA8JetColl            = cms.InputTag("goodPatJetsCA8OF"),
    input_card            = cms.string("/home/rsyarif/LJMet/CMSSW_7_0_7/src/ShowerDeconstruction/input_card.dat"), #how to put in $CMSSW_BASE/src/ShowerDeconstruction instead?
    #input card            = cms.FileInPath('ShowerDeconstruction/input_card.dat'),
    microjet_conesize     = cms.double(0.15),
    )


