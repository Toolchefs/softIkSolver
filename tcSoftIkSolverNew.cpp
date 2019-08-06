#include "tcSoftIkSolverNew.h"
#include <maya/MFnIkHandle.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MDoubleArray.h>
#include <maya/MIkHandleGroup.h>
#include <maya/MFnIkHandle.h>
#include <maya/MFnIkEffector.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MRampAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MPlugArray.h>
#include <maya/MGlobal.h>
#include <maya/MEulerRotation.h>

#include <math.h>

#define kFloatEpsilon        1.0e-5F
#define kDoubleEpsilon        1.0e-10
#define kDoubleEpsilonSqr    1.0e-20
#define kExtendedEpsilon    kDoubleEpsilon

MTypeId TcSoftIkSolverNodeNew::id(0x00122C00);

inline bool equivalent(double x, double y, double fudge = kDoubleEpsilon)
{
	return ((x > y) ? (x - y <= fudge) : (y - x <= fudge));
}

bool isParallel(const MVector &thisVector, const MVector &otherVector, double tolerance = kDoubleEpsilon)
{
	MVector v1, v2;
	v1 = thisVector.normal();
	v2 = otherVector.normal();
	double dotPrd = v1*v2;
	return (::equivalent(fabs(dotPrd), (double) 1.0, tolerance));
}

MVector getBoneScales(MFnIkJoint& jointFn)
{
	MPlug sxPlug = jointFn.findPlug("sx");
	MPlug syPlug = jointFn.findPlug("sy");
	MPlug szPlug = jointFn.findPlug("sz");
	double sxValue = sxPlug.asDouble();
	double syValue = syPlug.asDouble();
	double szValue = szPlug.asDouble();
	return MVector(sxValue, syValue, szValue);
}

TcSoftIkSolverNodeNew::TcSoftIkSolverNodeNew()
	: MPxIkSolverNode()
{

	setSingleChainOnly(true);
	setUniqueSolution(true);
	setPositionOnly(true);
	setMaxIterations(true);
}

TcSoftIkSolverNodeNew::~TcSoftIkSolverNodeNew() {}

void* TcSoftIkSolverNodeNew::creator()
{
	return new TcSoftIkSolverNodeNew;
}

MStatus TcSoftIkSolverNodeNew::initialize()
{
	return MS::kSuccess;
}

MString TcSoftIkSolverNodeNew::solverTypeName() const
{
	return MString(kSolverType);
}

MStatus setPointValue(MObject attr, MObject node, MVector& value)
{
	MPlug parentPlug(node, attr);

	if (parentPlug.isNull() && (parentPlug.numChildren()!=3))
		return MS::kFailure;

	MPlug xPlug = parentPlug.child(0);
	MPlug yPlug = parentPlug.child(1);
	MPlug zPlug = parentPlug.child(2);

	xPlug.setValue(value.x);
	yPlug.setValue(value.y);
	zPlug.setValue(value.z);

	return MS::kSuccess;
}

MVector getPointValue(MPlug parentPlug)
{
	MVector outVec;
	if (parentPlug.isNull() && (parentPlug.numChildren() != 3))
		return outVec;

	MPlug xPlug = parentPlug.child(0);
	MPlug yPlug = parentPlug.child(1);
	MPlug zPlug = parentPlug.child(2);

	xPlug.getValue(outVec.x);
	yPlug.getValue(outVec.y);
	zPlug.getValue(outVec.z);

	return outVec;
}

MTransformationMatrix::RotationOrder getRotationOrder(short value)
{
	switch (value)
	{
		case 0:
			return MTransformationMatrix::kInvalid;
		case 1:
			return MTransformationMatrix::kXYZ;		//!< \nop
		case 2:
			return MTransformationMatrix::kYZX;		//!< \nop
		case 3:
			return MTransformationMatrix::kZXY;		//!< \nop
		case 4:
			return MTransformationMatrix::kXZY;		//!< \nop
		case 5:
			return MTransformationMatrix::kYXZ;		//!< \nop
		case 6:
			return MTransformationMatrix::kZYX;		//!< \nop
		case 7:
			return MTransformationMatrix::kLast;		//!< \nop
		default:
			return MTransformationMatrix::kInvalid;
	}
}

MEulerRotation::RotationOrder getEulerRotationOrder(short value)
{
	switch (value)
	{
	case 0:
		return MEulerRotation::kXYZ;
	case 1:
		return MEulerRotation::kXYZ;		//!< \nop
	case 2:
		return MEulerRotation::kYZX;		//!< \nop
	case 3:
		return MEulerRotation::kZXY;		//!< \nop
	case 4:
		return MEulerRotation::kXZY;		//!< \nop
	case 5:
		return MEulerRotation::kYXZ;		//!< \nop
	case 6:
		return MEulerRotation::kZYX;		//!< \nop
	default:
		return MEulerRotation::kXYZ;
	}
}

short getShortRotationOrder(MTransformationMatrix::RotationOrder value)
{
	
	if (MTransformationMatrix::kInvalid == value)
		return 0;
	if (MTransformationMatrix::kXYZ == value)
		return 1;
	if (MTransformationMatrix::kYZX == value)
		return 2;
	if (MTransformationMatrix::kZXY == value)
		return 3;
	if (MTransformationMatrix::kXZY == value)
		return 4;
	if (MTransformationMatrix::kYXZ == value)
		return 5;
	if (MTransformationMatrix::kZYX == value)
		return 6;
	if (MTransformationMatrix::kLast == value)
		return 7;
	return MTransformationMatrix::kInvalid;
	
}



MMatrix getMidRestLocalMatrix(MFnIkHandle& handleFn, MVector parentScale = MVector(1.0, 1.0, 1.0))
{
	MMatrix outMtx;
	MPlug plug = handleFn.findPlug("mjs");
	MVector scale = getPointValue(plug);

	plug = handleFn.findPlug("mjr");
	MVector rotation = getPointValue(plug);

	short value = 1;
	plug = handleFn.findPlug("mjro");
	plug.getValue(value);
	MEulerRotation::RotationOrder rotOrder = getEulerRotationOrder(value);

	plug = handleFn.findPlug("mjt");
	MVector translation = getPointValue(plug);
	
	MMatrix parentInverScale;
	parentInverScale[0][0] = parentScale.x;
	parentInverScale[1][1] = parentScale.y;
	parentInverScale[2][2] = parentScale.z;
	parentInverScale = parentInverScale.inverse();
	
	plug = handleFn.findPlug("mjo");
	MVector jointOrient = getPointValue(plug);

	plug = handleFn.findPlug("mjra");
	MVector rotateAxis = getPointValue(plug);

	MTransformationMatrix mr1;
	mr1.rotateBy(MEulerRotation(rotation, rotOrder), MSpace::kTransform);

	MTransformationMatrix ms1;
	double scaleVec[3] = { scale.x, scale.y, scale.z };
	ms1.setScale(scaleVec, MSpace::kTransform);

	MTransformationMatrix mt1;
	mt1.setTranslation(translation, MSpace::kTransform);

	MTransformationMatrix mjo1;
	mjo1.rotateBy(MEulerRotation(jointOrient, rotOrder), MSpace::kTransform);

	MTransformationMatrix mra1;
	mra1.rotateBy(MEulerRotation(rotateAxis, rotOrder), MSpace::kTransform);

	outMtx = ms1.asMatrix() * mra1.asMatrix() * mr1.asMatrix() * mjo1.asMatrix() * parentInverScale * mt1.asMatrix();

	return outMtx;
}

MMatrix getEndRestLocalMatrix(MFnIkHandle& handleFn)
{
	MMatrix outMtx;
	MPlug plug = handleFn.findPlug("ejs");
	MVector scale = getPointValue(plug);

	plug = handleFn.findPlug("ejr");
	MVector rotation = getPointValue(plug);

	short value = 1;
	plug = handleFn.findPlug("ejro");
	plug.getValue(value);
	MEulerRotation::RotationOrder rotOrder = getEulerRotationOrder(value);

	plug = handleFn.findPlug("ejt");
	MVector translation = getPointValue(plug);

	plug = handleFn.findPlug("ejis");
	MVector parentScale = getPointValue(plug);
	MMatrix parentInverScale;
	parentInverScale[0][0] = parentScale.x;
	parentInverScale[1][1] = parentScale.y;
	parentInverScale[2][2] = parentScale.z;
	parentInverScale = parentInverScale.inverse();

	plug = handleFn.findPlug("ejo");
	MVector jointOrient = getPointValue(plug);

	plug = handleFn.findPlug("ejra");
	MVector rotateAxis = getPointValue(plug);

	MTransformationMatrix mr1;
	mr1.rotateBy(MEulerRotation(rotation, rotOrder), MSpace::kTransform);

	MTransformationMatrix ms1;
	double scaleVec[3] = { scale.x, scale.y, scale.z };
	ms1.setScale(scaleVec, MSpace::kTransform);

	MTransformationMatrix mt1;
	mt1.setTranslation(translation, MSpace::kTransform);

	MTransformationMatrix mjo1;
	mjo1.rotateBy(MEulerRotation(jointOrient, rotOrder), MSpace::kTransform);

	MTransformationMatrix mra1;
	mra1.rotateBy(MEulerRotation(rotateAxis, rotOrder), MSpace::kTransform);

	outMtx = ms1.asMatrix() * mra1.asMatrix() * mr1.asMatrix() * mjo1.asMatrix() * parentInverScale * mt1.asMatrix();

	return outMtx;
}

MStatus TcSoftIkSolverNodeNew::preSolve()
{
	MStatus status;
	setRotatePlane(true);
	setSingleChainOnly(true);
	setPositionOnly(true);

	MIkHandleGroup *handleGrp = handleGroup();
	MObject handle = handleGrp->handle(0);
	MDagPath handlePath = MDagPath::getAPathTo(handle);
	MFnIkHandle handleFn(handlePath, &status);

	MDagPath effectorPath;
	handleFn.getEffector(effectorPath);
	MFnIkEffector effectorFn(effectorPath);
	effectorPath.pop();
	MFnIkJoint midJointFn(effectorPath);

	MDagPath startJointPath;
	handleFn.getStartJoint(startJointPath);
	MFnIkJoint startJointFn(startJointPath);

	MDagPath lastJointPath = getEndJoint(handlePath);
	MFnIkJoint endJointFn(lastJointPath);

	if (!handleFn.hasAttribute("asft", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("activateSoft", "asft", MFnNumericData::kBoolean, true);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	if (!handleFn.hasAttribute("sftd", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("softDistance", "sftd", MFnNumericData::kDouble, 1.0);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	if (!handleFn.hasAttribute("astc", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("activateStretch", "astc", MFnNumericData::kDouble, 1.0);
		fnAttr.setMin(0.0);
		fnAttr.setMax(1.0);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	if (!handleFn.hasAttribute("mjsld", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("midJointSlide", "mjsld", MFnNumericData::kDouble, 0.0);
		fnAttr.setMax(1.0);
		fnAttr.setMin(-1.0);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	bool initRestPose = false;

	if (!handleFn.hasAttribute("mjlockw", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("midJointLockWeight", "mjlockw", MFnNumericData::kDouble, 0.0);
		fnAttr.setMin(0.0);
		fnAttr.setMax(1.0);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	if (!handleFn.hasAttribute("mjlockp", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("midJointLockPosition", "mjlockp");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	if (!handleFn.hasAttribute("plLock", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("usePoleVectorAsLockPosition", "plLock", MFnNumericData::kBoolean, false);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);
	}

	if (!handleFn.hasAttribute("mjs", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("midJointScale", "mjs");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		double scale[3] = { 0.0, 0.0, 0.0 };
		midJointFn.getScale(scale);
		setPointValue(attr, handleFn.object(), MVector(scale[0], scale[1], scale[2]));
	}
	MTransformationMatrix::RotationOrder rotateOrder = MTransformationMatrix::kXYZ;
	if (!handleFn.hasAttribute("mjro", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("midJointRotateOrder", "mjro", MFnNumericData::kShort, 1);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		MPlug plug(handleFn.object(), attr);
		rotateOrder = midJointFn.rotationOrder();
		plug.setValue(getShortRotationOrder(rotateOrder));
	}

	if (!handleFn.hasAttribute("mjr", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("midJointRotate", "mjr");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		double rotate[3] = { 0.0, 0.0, 0.0 };
		midJointFn.getRotation(rotate, rotateOrder);
		setPointValue(attr, handleFn.object(), MVector(rotate[0], rotate[1], rotate[2]));
	}

	if (!handleFn.hasAttribute("mjt", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("midJointTranslate", "mjt");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		setPointValue(attr, handleFn.object(), midJointFn.getTranslation(MSpace::kTransform));
	}

	if (!handleFn.hasAttribute("mjo", &status))
	{
		MFnNumericAttribute fnAttr;

		MObject attr = fnAttr.createPoint("midJointOrient", "mjo");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);

		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		MQuaternion orientation;
		midJointFn.getOrientation(orientation);
		MEulerRotation rot = orientation.asEulerRotation();
		setPointValue(attr, handleFn.object(), MVector(rot.x, rot.y, rot.z));
	}

	if (!handleFn.hasAttribute("mjra", &status))
	{
		MFnNumericAttribute fnAttr;
		MFnCompoundAttribute cAttr;
		MObject attr = fnAttr.createPoint("midJointRotateAxis", "mjra");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);

		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		MQuaternion orientation;
		midJointFn.getScaleOrientation(orientation);
		MEulerRotation rot = orientation.asEulerRotation();
		setPointValue(attr, handleFn.object(), MVector(rot.x, rot.y, rot.z));

	}

	if (!handleFn.hasAttribute("mjis", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("midJointParentInverseScale", "mjis");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		double scale[3] = { 0.0, 0.0, 0.0 };
		startJointFn.getScale(scale);
		setPointValue(attr, handleFn.object(), MVector(scale[0], scale[1], scale[2]));
	}

	// end joint
	if (!handleFn.hasAttribute("ejs", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("endJointScale", "ejs");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		double scale[3] = { 0.0, 0.0, 0.0 };
		endJointFn.getScale(scale);
		setPointValue(attr, handleFn.object(), MVector(scale[0], scale[1], scale[2]));
	}
	
	if (!handleFn.hasAttribute("ejro", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.create("endJointRotateOrder", "ejro", MFnNumericData::kShort, 1);
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		MPlug plug(handleFn.object(), attr);
		rotateOrder = endJointFn.rotationOrder();
		plug.setValue(getShortRotationOrder(rotateOrder));
	}

	if (!handleFn.hasAttribute("ejr", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("endJointRotate", "ejr");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		double rotate[3] = { 0.0, 0.0, 0.0 };
		endJointFn.getRotation(rotate, rotateOrder);
		setPointValue(attr, handleFn.object(), MVector(rotate[0], rotate[1], rotate[2]));
	}

	if (!handleFn.hasAttribute("ejt", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("endJointTranslate", "ejt");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		setPointValue(attr, handleFn.object(), endJointFn.getTranslation(MSpace::kTransform));
	}

	if (!handleFn.hasAttribute("ejo", &status))
	{
		MFnNumericAttribute fnAttr;

		MObject attr = fnAttr.createPoint("endJointOrient", "ejo");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);

		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		MQuaternion orientation;
		endJointFn.getOrientation(orientation);
		MEulerRotation rot = orientation.asEulerRotation();
		setPointValue(attr, handleFn.object(), MVector(rot.x, rot.y, rot.z));
	}

	if (!handleFn.hasAttribute("ejra", &status))
	{
		MFnNumericAttribute fnAttr;
		MFnCompoundAttribute cAttr;
		MObject attr = fnAttr.createPoint("endJointRotateAxis", "ejra");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);

		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		MQuaternion orientation;
		endJointFn.getScaleOrientation(orientation);
		MEulerRotation rot = orientation.asEulerRotation();
		setPointValue(attr, handleFn.object(), MVector(rot.x, rot.y, rot.z));

	}

	if (!handleFn.hasAttribute("ejis", &status))
	{
		MFnNumericAttribute fnAttr;
		MObject attr = fnAttr.createPoint("endJointParentInverseScale", "ejis");
		fnAttr.setKeyable(1);
		fnAttr.setWritable(1);
		fnAttr.setStorable(1);
		fnAttr.setReadable(1);
		handleFn.addAttribute(attr, MFnDependencyNode::kLocalDynamicAttr);

		double scale[3] = { 0.0, 0.0, 0.0 };
		midJointFn.getScale(scale);
		setPointValue(attr, handleFn.object(), MVector(scale[0], scale[1], scale[2]));
	}

	return MS::kSuccess;
}

MStatus TcSoftIkSolverNodeNew::doSolve()
{
	MStatus stat;
	// Handle Group
	//
	MIkHandleGroup * handle_group = handleGroup();
	if (NULL == handle_group) {
		return MS::kFailure;
	}

	// Handle
	//
	// For single chain types of solvers, get the 0th handle.
	// Single chain solvers are solvers which act on one handle only,
	// i.e. the    handle group for a single chain solver
	// has only one handle
	//
	MObject handle = handle_group->handle(0);
	MDagPath handlePath = MDagPath::getAPathTo(handle);
	MFnIkHandle handleFn(handlePath, &stat);

	// Effector
	//
	MDagPath effectorPath;
	handleFn.getEffector(effectorPath);
	MFnIkEffector effectorFn(effectorPath);

	// Mid Joint
	//
	effectorPath.pop();
	MFnIkJoint midJointFn(effectorPath);

	// Start Joint
	//
	MDagPath startJointPath;
	handleFn.getStartJoint(startJointPath);
	MFnIkJoint startJointFn(startJointPath);

	MDagPath lastJointPath = getEndJoint(handlePath);
	MFnIkJoint endJointFn(lastJointPath);

	MPlug sftdPlug = handleFn.findPlug("sftd");
	double m_softDistance = sftdPlug.asDouble();

	MPlug doSoftPlug = handleFn.findPlug("asft");
	double m_doSoft = doSoftPlug.asBool();

	MPlug doStretchPlug = handleFn.findPlug("astc");
	double m_doStretch = doStretchPlug.asDouble();

	MPlug elbowSlidePlug = handleFn.findPlug("mjsld");
	double m_elbowSlide = elbowSlidePlug.asDouble();

	MPlug elbowLockWeigthPlug = handleFn.findPlug("mjlockw");
	double m_elbowLockWeigth = elbowLockWeigthPlug.asDouble();


	double scale[3] = { 1.0, 1.0, 1.0 };
	startJointFn.getScale(scale);
	MMatrix midJointStartMatrix = getMidRestLocalMatrix(handleFn, MVector(scale[0], scale[1], scale[2]));
	MMatrix endJointStartMatrix = getEndRestLocalMatrix(handleFn);

	MMatrix midWorldMatrix = midJointStartMatrix * startJointPath.inclusiveMatrix();
	MMatrix endWorldMatrix = endJointStartMatrix * midWorldMatrix;

	MPoint handlePos = handleFn.rotatePivot(MSpace::kWorld);
	//MPoint effectorPos = effectorFn.rotatePivot(MSpace::kWorld);
	//MPoint midJointPos = midJointFn.rotatePivot(MSpace::kWorld);
	MPoint startJointPos = startJointFn.rotatePivot(MSpace::kWorld);
	MPoint effectorPos(endWorldMatrix[3][0], endWorldMatrix[3][1], endWorldMatrix[3][2]);
	MPoint midJointPos(midWorldMatrix[3][0], midWorldMatrix[3][1], midWorldMatrix[3][2]);

	
	// slide joint
	MPoint slideMidPosition;
	MVector globalVec = effectorPos - startJointPos;
	MVector midVec = midJointPos - startJointPos;
	MVector endVec = effectorPos - midJointPos;
	MVector dispVec;
	if (m_elbowSlide < 0.0)
	{
		dispVec = (globalVec.normal() * midVec.normal()) * m_elbowSlide * midVec.length() * globalVec.normal();
	}
	else
	{
		dispVec = (globalVec.normal() * endVec.normal()) * m_elbowSlide * endVec.length() * globalVec.normal();
	}


	MVector vector1 = midVec + dispVec;
	double newMidLength = vector1.length();
	MVector vector2 = globalVec - vector1;
	double newEndLength = vector2.length();

	double newStartAngle = globalVec.normal().angle(vector1.normal());
	double newMidAngle = vector1.normal().angle(vector2.normal());

	double oldStartAngle = globalVec.normal().angle((midJointPos - startJointPos).normal());
	double oldMidAngle = (midJointPos - startJointPos).normal().angle((effectorPos - midJointPos).normal());

	MVector startCross = globalVec.normal() ^ (midJointPos - startJointPos).normal();
	startCross.normalize();
	MQuaternion qStartElbow(newStartAngle - oldStartAngle, startCross);

	MVector midCross = (midJointPos - startJointPos).normal() ^ (effectorPos - midJointPos).normal();
	midCross.normalize();
	MQuaternion qMidElbow(newMidAngle - oldMidAngle, midCross);

	//midJointFn.rotateBy(qMidElbow, MSpace::kWorld);
	//startJointFn.rotateBy(qStartElbow, MSpace::kWorld);

	MPlug plug2 = handleFn.findPlug("ejt");
	MVector effectorTranslation2 = getPointValue(plug2);

	plug2 = handleFn.findPlug("mjt");
	MVector midTranslation2 = getPointValue(plug2);

	MVector midTranslationOffset = midTranslation2;
	MVector effectorTranslationOffset = effectorTranslation2;
	midTranslationOffset = midTranslation2 - midTranslation2.normal() * newMidLength;
	effectorTranslationOffset = effectorTranslation2 - effectorTranslation2.normal() * newEndLength;
	//midJointFn.setTranslation(midTranslation, MSpace::kTransform);
	//endJointFn.setTranslation(effectorTranslation, MSpace::kTransform);
	
	// end slide joint
	
	


	MVector poleVector = poleVectorFromHandle(handlePath);
	poleVector *= handlePath.exclusiveMatrix();
	double twistValue = twistFromHandle(handlePath);

	MQuaternion qStart, qMid;

	// vector from startJoint to midJoint
	//MVector vector1 = midJointPos - startJointPos;
	// vector from midJoint to effector
	//MVector vector2 = effectorPos - midJointPos;
	// vector from startJoint to handle
	MVector vectorH = handlePos - startJointPos;
	// vector from startJoint to effector
	MVector vectorE = effectorPos - startJointPos;

	//double startScale2[3] = {m_initStartScale[0] * m_stretchRatio, m_initStartScale[1], m_initStartScale[2]};
	//double midScale2[3] = {m_initMidScale[0] * m_stretchRatio, m_initMidScale[1], m_initMidScale[2]};

	bool doingSoft = false;
	if ((m_doSoft) && (m_softDistance > 0.0) && (vectorH.length() > ((vector1.length() + vector2.length()) - m_softDistance)))
	{
		double dChain = (vector1.length() + vector2.length());
		double dSoft = m_softDistance;
		double dA = dChain - m_softDistance;
		double x = vectorH.length();
		vectorH = vectorH.normal() * (dA + dSoft * (1.0 - exp(-(x - dA) / dSoft)));
		doingSoft = true;
	}
	
	// lengths of those vectors
	double length1 = vector1.length();
	double length2 = vector2.length();
	double lengthH = vectorH.length();
	// component of the vector1 orthogonal to the vectorE
	MVector vectorO =
		vector1 - vectorE*((vector1*vectorE) / (vectorE*vectorE));

	//////////////////////////////////////////////////////////////////
	// calculate q12 which solves for the midJoint rotation
	//////////////////////////////////////////////////////////////////
	// angle between vector1 and vector2
	double vectorAngle12 = vector1.angle(vector2);
	// vector orthogonal to vector1 and 2
	MVector vectorCross12 = vector1^vector2;
	double lengthHsquared = lengthH*lengthH;
	// angle for arm extension
	double cos_theta =
		(lengthHsquared - length1*length1 - length2*length2)
		/ (2 * length1*length2);
	if (cos_theta > 1)
		cos_theta = 1;
	else if (cos_theta < -1)
		cos_theta = -1;
	double theta = acos(cos_theta);
	// quaternion for arm extension
	MQuaternion q12(theta - vectorAngle12, vectorCross12);

	//////////////////////////////////////////////////////////////////
	// calculate qEH which solves for effector rotating onto the handle
	//////////////////////////////////////////////////////////////////
	// vector2 with quaternion q12 applied
	vector2 = vector2.rotateBy(q12);
	// vectorE with quaternion q12 applied
	vectorE = vector1 + vector2;
	// quaternion for rotating the effector onto the handle
	MQuaternion qEH(vectorE, vectorH);

	//////////////////////////////////////////////////////////////////
	// calculate qNP which solves for the rotate plane
	//////////////////////////////////////////////////////////////////
	// vector1 with quaternion qEH applied
	vector1 = vector1.rotateBy(qEH);
	if (vector1.isParallel(vectorH))
		// singular case, use orthogonal component instead
		vector1 = vectorO.rotateBy(qEH);
	// quaternion for rotate plane
	MQuaternion qNP;
	if (!isParallel(poleVector, vectorH) && (lengthHsquared != 0)) {
		// component of vector1 orthogonal to vectorH
		MVector vectorN =
			vector1 - vectorH*((vector1*vectorH) / lengthHsquared);
		// component of pole vector orthogonal to vectorH
		MVector vectorP =
			poleVector - vectorH*((poleVector*vectorH) / lengthHsquared);
		double dotNP = (vectorN*vectorP) / (vectorN.length()*vectorP.length());
		if (absoluteValue(dotNP + 1.0) < kEpsilon) {
			// singular case, rotate halfway around vectorH
			MQuaternion qNP1(kPi, vectorH);
			qNP = qNP1;
		}
		else {
			MQuaternion qNP2(vectorN, vectorP);
			qNP = qNP2;
		}
	}

	//////////////////////////////////////////////////////////////////
	// calculate qTwist which adds the twist
	//////////////////////////////////////////////////////////////////
	MQuaternion qTwist(twistValue, vectorH);

	// quaternion for the mid joint
	qMid = q12;
	// concatenate the quaternions for the start joint
	qStart = qEH*qNP*qTwist;

	MVector midRotAxis;
	double midRot = 0.0;
	qMid.getAxisAngle(midRotAxis, midRot);
	midRotAxis = midRotAxis * midWorldMatrix.inverse();

	MPlug plug = handleFn.findPlug("mjr");
	MVector rotation = getPointValue(plug);

	short value = 1;
	plug = handleFn.findPlug("mjro");
	plug.getValue(value);
	MEulerRotation::RotationOrder rotOrder = getEulerRotationOrder(value);
	MEulerRotation mRot(rotation, rotOrder);

	//qMidElbow
	MVector midElbowRotAxis;
	double midElbowRot = 0.0;
	qMidElbow.getAxisAngle(midElbowRotAxis, midElbowRot);
	midElbowRotAxis = midElbowRotAxis * midWorldMatrix.inverse();
	
	midJointFn.setRotation(mRot.asQuaternion() * MQuaternion(midElbowRot, midElbowRotAxis) * MQuaternion(midRot, midRotAxis), MSpace::kTransform);
	startJointFn.rotateBy(qStart * qStartElbow, MSpace::kWorld);
	

	//midJointFn.setRotation(mRot.asQuaternion() * MQuaternion(midRot, midRotAxis), MSpace::kTransform);
	//startJointFn.rotateBy(qStart, MSpace::kWorld);
	

	//////////////////////////
	// Stretch
	/////////////////////////
	plug = handleFn.findPlug("ejt");
	MVector effectorTranslation = getPointValue(plug) -effectorTranslationOffset;

	plug = handleFn.findPlug("mjt");
	MVector midTranslation = getPointValue(plug) -midTranslationOffset;

	double m_stretchRatio = 1.0;
	double ratio = vectorH.length() / vectorE.length();
	MVector vectorH2 = handlePos - startJointPos;
	if (doingSoft)
	{
		ratio = vectorH2.length() / vectorH.length();
	}
	if ((m_doStretch > 0.0001) && (ratio > 1.0))
	{
		m_stretchRatio = 1.0 + (ratio - 1.0) * m_doStretch;
	}
	else
	{
		m_stretchRatio = 1.0;
	}

	effectorTranslation = effectorTranslation * m_stretchRatio;
	midTranslation = midTranslation * m_stretchRatio;

	endJointFn.setTranslation(effectorTranslation, MSpace::kTransform);
	midJointFn.setTranslation(midTranslation, MSpace::kTransform);
	
	/////////////////////////
	// Elbow lock
	/////////////////////////
	if (m_elbowLockWeigth > 0.0)
	{

		MVector m_elbowPos;
		MPlug elbowPosmjlockplug = handleFn.findPlug("plLock");
		if (elbowPosmjlockplug.asBool())
		{
			MVector poleVector = poleVectorFromHandle(handlePath);
			poleVector *= handlePath.exclusiveMatrix();
			m_elbowPos = startJointPos + poleVector;
		}
		else
		{
			MPlug elbowLockPosPlug = handleFn.findPlug("mjlockp");
			MPlug pXPlug = elbowLockPosPlug.child(0);
			MPlug pYPlug = elbowLockPosPlug.child(1);
			MPlug pZPlug = elbowLockPosPlug.child(2);

			m_elbowPos.x = pXPlug.asDouble();
			m_elbowPos.y = pYPlug.asDouble();
			m_elbowPos.z = pZPlug.asDouble();
		}

		MPlug endJointRestPosePlug = handleFn.findPlug("ejrp");
		MPlug endXPlug = endJointRestPosePlug.child(0);
		MPlug endYPlug = endJointRestPosePlug.child(1);
		MPlug endZPlug = endJointRestPosePlug.child(2);
		double endLength = MVector(endXPlug.asDouble(), endYPlug.asDouble(), endZPlug.asDouble()).length();

		MPoint handlePos3 = handleFn.rotatePivot(MSpace::kWorld);
		MPoint effectorPos3 = effectorFn.rotatePivot(MSpace::kWorld);
		MPoint midJointPos3 = midJointFn.rotatePivot(MSpace::kWorld);
		MPoint startJointPos3 = startJointFn.rotatePivot(MSpace::kWorld);

		MVector newFirstVector = m_elbowPos - startJointPos3 + (1.0 - m_elbowLockWeigth) * (midJointPos3 - m_elbowPos);
		MVector currVec1 = midJointPos3 - startJointPos3;
		MQuaternion newQ1(currVec1.normal(), newFirstVector.normal());

		startJointFn.rotateBy(newQ1, MSpace::kWorld);
		midJointFn.setTranslation(m_elbowPos + (1.0 - m_elbowLockWeigth) * (midJointPos3 - m_elbowPos), MSpace::kWorld);

		handlePos3 = handleFn.rotatePivot(MSpace::kWorld);
		effectorPos3 = endJointFn.rotatePivot(MSpace::kWorld);
		midJointPos3 = midJointFn.rotatePivot(MSpace::kWorld);
		startJointPos3 = startJointFn.rotatePivot(MSpace::kWorld);

		MVector newSecondVector = handlePos3 - midJointPos3;
		MVector currVec2 = effectorPos3 - midJointPos3;
		MQuaternion newQ2(currVec2.normal(), newSecondVector.normal());
		midJointFn.rotateBy(newQ2, MSpace::kWorld);
		endJointFn.setTranslation(endJointFn.translation(MSpace::kTransform).normal() * (endLength + (m_doStretch * (newSecondVector.length() - endLength))), MSpace::kTransform);
	}
	
	
	return MS::kSuccess;
}

MStatus TcSoftIkSolverNodeNew::postSolve(MStatus status)
{
	

	return MS::kSuccess;
}


MVector TcSoftIkSolverNodeNew::poleVectorFromHandle(const MDagPath &handlePath)
//
// This method returns the pole vector of the IK handle.
//
{
	MStatus stat;
	MFnIkHandle handleFn(handlePath, &stat);
	MPlug pvxPlug = handleFn.findPlug("pvx");
	MPlug pvyPlug = handleFn.findPlug("pvy");
	MPlug pvzPlug = handleFn.findPlug("pvz");
	double pvxValue, pvyValue, pvzValue;
	pvxPlug.getValue(pvxValue);
	pvyPlug.getValue(pvyValue);
	pvzPlug.getValue(pvzValue);
	MVector poleVector(pvxValue, pvyValue, pvzValue);
	return poleVector;
}

double TcSoftIkSolverNodeNew::twistFromHandle(const MDagPath &handlePath)
//
// This method returns the twist of the IK handle.
//
{
	MStatus stat;
	MFnIkHandle handleFn(handlePath, &stat);
	MPlug twistPlug = handleFn.findPlug("twist");
	double twistValue;
	twistPlug.getValue(twistValue);
	return twistValue;
}

MDagPath TcSoftIkSolverNodeNew::getEndJoint(const MDagPath &handlePath)
{
	MStatus status;

	MFnIkHandle handleFn(handlePath, &status);
	// Effector
	//
	MDagPath effectorPath;
	handleFn.getEffector(effectorPath);
	MFnIkEffector effectorFn(effectorPath);

	MPlug effectorPlug = effectorFn.findPlug("tx");
	MPlugArray plugArr;
	MObject jointNode;
	if (effectorPlug.connectedTo(plugArr, true, false, &status) && (plugArr.length() > 0))
	{
		if (plugArr[0].node().hasFn(MFn::kJoint))
		{
			jointNode = plugArr[0].node();
		}
	}
	else
	{
		effectorPlug = effectorFn.findPlug("ty");
		if (effectorPlug.connectedTo(plugArr, true, false, &status) && (plugArr.length() > 0))
		{
			if (plugArr[0].node().hasFn(MFn::kJoint))
			{
				jointNode = plugArr[0].node();
			}
		}
		else
		{
			effectorPlug = effectorFn.findPlug("tz");
			if (effectorPlug.connectedTo(plugArr, true, false, &status) && (plugArr.length() > 0))
			{
				if (plugArr[0].node().hasFn(MFn::kJoint))
				{
					jointNode = plugArr[0].node();
				}
			}
		}
	}
	// slide joint
	MDagPath lastJointPath;
	effectorPath.pop();
	for (unsigned int i = 0; i < effectorPath.childCount(); ++i)
	{
		MDagPath currJointPath = effectorPath;
		MObject child = effectorPath.child(i);
		currJointPath.push(child);
		if (child.hasFn(MFn::kJoint))
		{
			lastJointPath = currJointPath;
			if (lastJointPath.node() == jointNode)
			{
				break;
			}
		}
	}

	return lastJointPath;
}