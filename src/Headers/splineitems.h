//
// splineitems.h
//




//=====================================================

void PrimeSplines(void);
void GetObjectCoordOnSpline(ObjNode *theNode);
void GetCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z);
void GetNextCoordOnSpline(SplineDefType *splinePtr, float placement, float *x, float *z);
Boolean IsSplineItemVisible(ObjNode *theNode);
void AddToSplineObjectList(ObjNode *theNode, Boolean setAim);
void MoveSplineObjects(void);
Boolean RemoveFromSplineObjectList(ObjNode *theNode);
void EmptySplineObjectList(void);
void IncreaseSplineIndex(ObjNode *theNode, float speed);
void IncreaseSplineIndexZigZag(ObjNode *theNode, float speed);
void DetachObjectFromSpline(ObjNode *theNode, void *moveCall);
void SetSplineAim(ObjNode *theNode);















