//
// Copyright (C) toolchefs 
// 
// File: pluginMain.cpp
//
// Author: Maya Plug-in Wizard 2.0
//

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include "tcSoftIkSolverNew.h"
#include "addTcSoftIkSolverCallbacks.h"


MStatus initializePlugin( MObject obj )
//
//	Description:
//		this method is called when the plug-in is loaded into Maya.  It 
//		registers all of the services that this plug-in provides with 
//		Maya.
//
//	Arguments:
//		obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{ 
	MFnPlugin plugin( obj, "Toolchefs_tcSoftIkSolver", "1.0", "Any");

	MStatus status = MStatus::kSuccess;
	MString errorString;

	status = plugin.registerNode("tcSoftIkSolver", 
		TcSoftIkSolverNodeNew::id,
		&TcSoftIkSolverNodeNew::creator,
		&TcSoftIkSolverNodeNew::initialize,
								 MPxNode::kIkSolverNode);
	if(status != MS::kSuccess) {
		status.perror("registerNode");
		return status;
	}

	status = plugin.registerCommand("addTcSoftIkSolverCallbacks",
	                                addTcSoftIkSolverCallbacks::creator);
	if(status != MS::kSuccess) {
		status.perror("registerCommand");
		return status;
	}

	MGlobal::executeCommand("addTcSoftIkSolverCallbacks;if(!`objExists tcSoftIkSolver`){createNode -n tcSoftIkSolver tcSoftIkSolver;};",false,false);
	return status;
}

MStatus uninitializePlugin( MObject obj )
//
//	Description:
//		this method is called when the plug-in is unloaded from Maya. It 
//		deregisters all of the services that it was providing.
//
//	Arguments:
//		obj - a handle to the plug-in object (use MFnPlugin to access it)
//
{
	MStatus   status;
	MFnPlugin plugin( obj );

	status = plugin.deregisterNode(TcSoftIkSolverNodeNew::id);
	if (!status) {
		status.perror("deregisterNode");
		return status;
	}

	status = plugin.deregisterCommand("addTcSoftIkSolverCallbacks");
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}
	
	// Remove callbacks when plug-in is unloaded.
	MMessage::removeCallback(addTcSoftIkSolverCallbacks::afterNewId);
	MMessage::removeCallback(addTcSoftIkSolverCallbacks::afterOpenId);

	return status;
}
