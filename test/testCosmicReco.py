import FWCore.ParameterSet.Config as cms

process = cms.Process("CREC")
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.destinations = cms.untracked.vstring('cout')
process.MessageLogger.categories = cms.untracked.vstring("CosmicMuonTrajectoryBuilder")
process.MessageLogger.debugModules = cms.untracked.vstring("*")
process.MessageLogger.cout = cms.untracked.PSet(
           threshold = cms.untracked.string('DEBUG'),
           INFO = cms.untracked.PSet(
             limit = cms.untracked.int32(0)
           ),
          DEBUG = cms.untracked.PSet(
             limit = cms.untracked.int32(0)
          ),
	   CosmicMuonTrajectoryBuilder = cms.untracked.PSet(
             limit = cms.untracked.int32(10000000)
           )
     ) 

process.load("CondCore.DBCommon.CondDBSetup_cfi")

process.maxEvents = cms.untracked.PSet(  input = cms.untracked.int32(10) )
process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring(
        '/store/data/BeamCommissioning08/BeamHalo/RECO/v1/000/062/241/466EDC3D-8180-DD11-B2FD-000423D94700.root',
        '/store/data/BeamCommissioning08/BeamHalo/RECO/v1/000/062/242/D65A4551-8380-DD11-BA0C-000423D9863C.root'
    )
)

# output module
#
process.load("Configuration.EventContent.EventContentCosmics_cff")

process.FEVT = cms.OutputModule("PoolOutputModule",
    process.FEVTEventContent,
    dataset = cms.untracked.PSet(dataTier = cms.untracked.string('RECO')),
    fileName = cms.untracked.string('/tmp/cliu/promptrecoCosmics.root'),
    SelectEvents = cms.untracked.PSet(
         SelectEvents = cms.vstring('allPath')
    )
)

process.FEVT.outputCommands.append('keep *_*_*_CREC')

# Conditions (Global Tag is used here):
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.GlobalTag.connect = "frontier://PromptProd/CMS_COND_21X_GLOBALTAG"
process.GlobalTag.globaltag = "STARTUP_V7::All"
process.prefer("GlobalTag")

# Magnetic fiuld: force mag field to be 0 tesla
process.load("Configuration.StandardSequences.MagneticField_0T_cff")

#Geometry
process.load("Configuration.StandardSequences.Geometry_cff")

# reconstruction sequence for Cosmics
process.load("RecoMuon.MuonSeedGenerator.CosmicMuonSeedProducer_cfi")

process.load("RecoMuon.CosmicMuonProducer.cosmicMuons_cff")

process.cosmic = cms.Sequence( process.CosmicMuonSeed * process.cosmicMuons )

process.allPath = cms.Path( process.cosmic ) 

process.outpath = cms.EndPath(process.FEVT)
