#include "addTcSoftIkSolverCallbacks.h"

#include <maya/MSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>



MCallbackId addTcSoftIkSolverCallbacks::afterNewId;
MCallbackId addTcSoftIkSolverCallbacks::afterOpenId;															

void *addTcSoftIkSolverCallbacks::creator()
{
	return new addTcSoftIkSolverCallbacks;
}

void createIK2BsolverAfterNew(void *clientData)
//
// This method creates the ik2Bsolver after a File->New.
//
{
	MSelectionList selList;
	MGlobal::getActiveSelectionList( selList );
	MGlobal::executeCommand("if(!`objExists tcSoftIkSolver`){createNode -n tcSoftIkSolver tcSoftIkSolver;};");
	MGlobal::setActiveSelectionList( selList );
}

void createIK2BsolverAfterOpen(void *clientData)
//
// This method creates the ik2Bsolver after a File->Open
// if the ik2Bsolver does not exist in the loaded file.
//
{
	MSelectionList selList;
	MGlobal::getSelectionListByName("tcSoftIkSolver", selList);
	if (selList.length() == 0) {
		MGlobal::getActiveSelectionList( selList );
		MGlobal::executeCommand("if(!`objExists tcSoftIkSolver`){createNode -n tcSoftIkSolver tcSoftIkSolver;};");
		MGlobal::setActiveSelectionList( selList );
	}
}

MStatus addTcSoftIkSolverCallbacks::doIt(const MArgList &args)
//
// This method adds the File->New and File->Open callbacks
// used to recreate the ik2Bsolver.
//
{
    // Get the callback IDs so we can deregister them 
	// when the plug-in is unloaded.
	afterNewId = MSceneMessage::addCallback(MSceneMessage::kAfterNew, 
							   createIK2BsolverAfterNew);
	afterOpenId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, 
							   createIK2BsolverAfterOpen);
	return MS::kSuccess;
}
