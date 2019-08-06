#pragma once

#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MCallbackIdArray.h>


class addTcSoftIkSolverCallbacks : public MPxCommand
{
public:															

	addTcSoftIkSolverCallbacks() {};

	virtual MStatus	doIt (const MArgList &);

	static void*	creator();
	
	// callback IDs for the solver callbacks
	static MCallbackId afterNewId;

	static MCallbackId afterOpenId;
};
