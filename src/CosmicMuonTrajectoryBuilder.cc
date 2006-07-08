#include "RecoMuon/CosmicMuonProducer/interface/CosmicMuonTrajectoryBuilder.h"
/** \file CosmicMuonTrajectoryBuilder
 *
 *  class to build trajectories of muons from cosmic rays
 *  using DirectMuonNavigation
 *
 *  $Date: 2006/07/03 01:10:52 $
 *  $Revision: 1.3 $
 *  \author Chang Liu  - Purdue Univeristy
 */


#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/DTRecHit/interface/DTRecSegment4D.h"

/* Collaborating Class Header */
#include "TrackingTools/KalmanUpdators/interface/KFUpdator.h"
#include "TrackingTools/KalmanUpdators/interface/Chi2MeasurementEstimator.h"
#include "TrackingTools/TrajectoryState/interface/TrajectoryStateTransform.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"
#include "TrackingTools/PatternTools/interface/Trajectory.h"
#include "TrackPropagation/SteppingHelixPropagator/interface/SteppingHelixPropagator.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"
#include "DataFormats/TrajectorySeed/interface/TrajectorySeed.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "TrackingTools/DetLayers/interface/DetLayer.h"
#include "RecoMuon/Records/interface/MuonRecoGeometryRecord.h"
#include "RecoMuon/Navigation/interface/DirectMuonNavigation.h"
#include "RecoMuon/MeasurementDet/interface/MuonDetLayerMeasurements.h"
#include "RecoMuon/TrackingTools/interface/MuonBestMeasurementFinder.h"
#include "RecoMuon/TrackingTools/interface/MuonTrajectoryUpdator.h"
#include "Geometry/CommonDetUnit/interface/GlobalTrackingGeometry.h"
#include "Geometry/Records/interface/GlobalTrackingGeometryRecord.h"
#include "RecoMuon/TransientTrackingRecHit/interface/MuonTransientTrackingRecHit.h"
#include "DataFormats/MuonDetId/interface/MuonSubdetId.h"


CosmicMuonTrajectoryBuilder::CosmicMuonTrajectoryBuilder(const edm::ParameterSet& par) 
{
  edm::LogInfo ("CosmicMuonTrajectoryBuilder")<< "CosmicMuonTrajectoryBuilder begin";
  theMaxChi2 = par.getParameter<double>("MaxChi2");;
  theEstimator = new Chi2MeasurementEstimator(theMaxChi2);
  theLayerMeasurements = new MuonDetLayerMeasurements(true, true, "DTSegment4DProducer","CSCSegmentProducer");


}

CosmicMuonTrajectoryBuilder::~CosmicMuonTrajectoryBuilder() {
  edm::LogInfo ("CosmicMuonTrajectoryBuilder")<< "CosmicMuonTrajectoryBuilder end";
  delete theUpdator;
  delete theBestMeasurementFinder; 
  delete theEstimator;
//  delete thePropagator;
// @@CL: FIXME: why crashed if delete propagator?
}

void CosmicMuonTrajectoryBuilder::setES(const edm::EventSetup& setup) {

  setup.get<GlobalTrackingGeometryRecord>().get(theTrackingGeometry); 
  setup.get<IdealMagneticFieldRecord>().get(theField);
  setup.get<MuonRecoGeometryRecord>().get(theDetLayerGeometry);

  thePropagator = new SteppingHelixPropagator(&*theField, alongMomentum);
  theBestMeasurementFinder = new MuonBestMeasurementFinder(thePropagator);
  theUpdator = new MuonTrajectoryUpdator(thePropagator, theMaxChi2, 0);

}

void CosmicMuonTrajectoryBuilder::setEvent(const edm::Event& event) {

  theLayerMeasurements->setEvent(event);

}

MuonTrajectoryBuilder::TrajectoryContainer 
CosmicMuonTrajectoryBuilder::trajectories(const TrajectorySeed& seed){

  std::vector<Trajectory> trajL;
  TrajectoryStateTransform tsTransform;

  DirectMuonNavigation navigation(&*theDetLayerGeometry);
 
  PTrajectoryStateOnDet ptsd1(seed.startingState());
  DetId did(ptsd1.detId());
  const BoundPlane& bp = theTrackingGeometry->idToDet(did)->surface();
  TrajectoryStateOnSurface lastTsos = tsTransform.transientState(ptsd1,&bp,&*theField);
// use a larger error if it's in DT Chambers
// and if only z or phi
// since the direction does not contain enough information
// FIXME 
//  if ( (did.subdetId() == MuonSubdetId::DT) && (seed.nHits() ==1) ) {
//    const TrackingRecHit* hit = &*(seed.recHits().first);
//    const DTRecSegment4D * seg =dynamic_cast<const DTRecSegment4D*>(hit); 
//    if (seg) {
//      if (!(seg->hasZed()&&seg->hasPhi())) 
//      lastTsos.rescaleError(10.0);
//    }
//  }

   vector<const DetLayer*> navLayerCBack = navigation.compatibleLayers(*(lastTsos.freeState()), oppositeToMomentum);

  if (navLayerCBack.size() == 0) {
    return std::vector<Trajectory>();
  }  
  Trajectory theTraj(seed);

  int DTChamberUsedBack = 0;
  int CSCChamberUsedBack = 0;
  int RPCChamberUsedBack = 0;
  int TotalChamberUsedBack = 0;

  for ( vector<const DetLayer*>::const_iterator rnxtlayer = navLayerCBack.begin(); rnxtlayer!= navLayerCBack.end(); ++rnxtlayer) {

     vector<TrajectoryMeasurement> measL =
        theLayerMeasurements->measurements(*rnxtlayer, lastTsos, *propagator(), *estimator());
     if (measL.size()==0 ) continue;
     TrajectoryMeasurement* theMeas=measFinder()->findBestMeasurement(measL);
     
      if ( theMeas ) {

        pair<bool,TrajectoryStateOnSurface> result
            = updator()->update(theMeas, theTraj);

        if (result.first) {
          if((*rnxtlayer)->module()==dt) DTChamberUsedBack++;
          else if((*rnxtlayer)->module()==csc) CSCChamberUsedBack++;
          else if((*rnxtlayer)->module()==rpc) RPCChamberUsedBack++;
          TotalChamberUsedBack++;

          if ( !theTraj.empty() ) lastTsos = result.second;
          else lastTsos = theMeas->predictedState();
        }//if result.first
      }//if the meas
  } //for layers

  if (theTraj.isValid() && TotalChamberUsedBack>=2 && (DTChamberUsedBack+CSCChamberUsedBack)>0){

     trajL.push_back(theTraj);
    } //if traj valid

  return trajL;
}

