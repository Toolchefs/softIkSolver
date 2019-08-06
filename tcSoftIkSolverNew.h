#pragma once

#pragma once

#include <maya/MPxIkSolverNode.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MDagPath.h>

#define MAX_ITERATIONS 1
#define kSolverType "softIkSolverNode"
#define kPi 3.14159265358979323846264338327950
#define kEpsilon 1.0e-5
#define absoluteValue(x) ((x) < 0 ? (-(x)) : (x))

class TcSoftIkSolverNodeNew : public MPxIkSolverNode {
public:
	TcSoftIkSolverNodeNew();

	virtual ~TcSoftIkSolverNodeNew();

	virtual MStatus preSolve();

	virtual MStatus doSolve();

	virtual MStatus postSolve(MStatus stat);

	virtual MString solverTypeName() const;

	static void* creator();

	static MStatus initialize();

	static MTypeId id;


private:

	MStatus doSimpleSolver();

	MVector poleVectorFromHandle(const MDagPath &handlePath);

	MDagPath getEndJoint(const MDagPath &handlePath);

	double    twistFromHandle(const MDagPath &handlePath);

};
