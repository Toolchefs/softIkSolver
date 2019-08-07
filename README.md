## Maya Soft IK Solver

Our maya IK solver is an advanced solution for 2 bones setup. It fixes the annoying “pop” problem at full extension in the normal IK system. Moreover it implements a full controllable stretch algorithm and a “elbow slide” and “elbow lock” system needed by modern rigs. The advantage of our solver is that no extra nodes are needed and for this reason is faster than current solutions. It’s possible to update old rigs without creating new nodes or modifying the setup, just load our plugin and choose the tcSoftIKSolver from the ikHandle.

Features:

* Soft extension
* Stretch
* Elbow slide
* Elbow lock

If you are planning to use one of our tools in a studio, we would be grateful if you could let us know.


## How to use:

Load the tcSoftIkSolver plug-in from the Maya plug-ins window. The plug-in will automatically add a tcSoftIkSolver node in the following cases: when a scene is opened, when a scene is created, when the plugin is loaded.

Once the tcSoftIkSolver will be selected as “IK Solver” for an ikHandle, some attributes will be added to the same ikHandle:

* Activate soft: enable/disable the soft extension. When this is disabled, the solver works like the normal maya ik, where there is the “pop” problem as the ik approaches the full extension. When this is enabled the velocity of the joints that approach the full extension position is damped by an exponential function controlled by the “soft distance” attribute. The solver starts to use this exponential function when the distance between the first joint and the ikHandle is equal to the full extension, less the “soft distance”.
* Soft distance: Distance used by the exponential function of the “soft” algorithm to damp the extension of the chain
* Activate stretch: enable/disable the stretch of the joint. When enabled the joints will be stretched in order to always reach the ikHandle position
* Mid joint slide: This attribute add an offset to the middle joint in the chain, the offset is parallel to the vector between the start joint and the end joint. Only the middle joint of the chain is moved, while the start and end joint remain in the same position
* Mid joint lock weight: This control the blend between the position of the mid joint computed by the nromal ik algorithm and the position given by the “Mid joint lock position” or by the pole vector if the attribute “Use pole vector as lock position” is checked
* Mid joint lock position: This is the position used to constriant the position of the mid joint when the “Mid joint lock weight” attribute is greater than 0.0
* Use pole vector as lock position: The pole vector is used instead the “Mid joint lock position” to constraint the mid joint
* Mid/End joint scale: mid/end joint scale in rest position
* Mid/End joint rotate order: mid/end joint rotate order in rest position
* Mid/End joint rotate: mid/end joint rotation in rest position
* Mid/End joint translate: mid/end joint translation in rest position
* Mid/End joint orient: mid/end joint orient in rest position
* Mid/End joint rotate axis: mid/end joint rotate axis in rest position
* Mid/End joint parent inverse scale: mid/end joint parent scale in rest position


## Known limitations:

* The Soft Ik solver works only with chains of 2 bones.
* When using Viewport 2.0, the drawing of the joints may be incorrect. The legacy viewport doesn’t have this issue and it will always draw the joints correctly.
* Due to a bug in the maya undo system when some constraint are attached directly or indirectly to the joints or to the ikHandle, sometimes maya doesn’t refresh the correct position when and undo is executed. We reported this bug to autodesk we’ll let you know when they will fix it.

## License

This project is licensed under [the LPGP license](http://www.gnu.org/licenses/).

## Contact us

Please feel free to contact us at support@toolchefs.com in case you would like contribute or simply have questions.

### ENJOY!
