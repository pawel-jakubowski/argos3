#include "physx_differential_drive.h"
#include <argos3/plugins/simulator/physics_engines/physx/physx_multi_body_object_model.h>

namespace argos {

   /****************************************/
   /****************************************/

   CPhysXDifferentialDrive::CPhysXDifferentialDrive(
      CPhysXMultiBodyObjectModel& c_model,
      CPhysXEngine& c_physx_engine,
      physx::PxReal f_interwheel_distance,
      physx::PxReal f_wheel_radius,
      physx::PxReal f_wheel_thickness,
      physx::PxReal f_wheel_mass,
      const physx::PxVec3& c_body_size,
      physx::PxReal f_body_elevation,
      physx::PxReal f_body_mass) :
      m_cPhysXEngine(c_physx_engine),
      m_fWheelRadius(f_wheel_radius),
      m_fWheelThickness(f_wheel_thickness) {
      /*
       * Calculate offsets
       */
      /* Main body */
      m_cMainBodyOffset =
         physx::PxTransform(0.0f,
                            0.0f,
                            c_body_size.z * 0.5f + f_body_elevation);
      /* Left wheel */
      m_cLeftWheelOffset =
         physx::PxTransform(0.0f,
                            f_interwheel_distance * 0.5f,
                            m_fWheelRadius,
                            physx::PxQuat(-CRadians::PI_OVER_TWO.GetValue(),
                                          physx::PxVec3(1.0f, 0.0f, 0.0f)));
      /* Right wheel */
      m_cRightWheelOffset =
         physx::PxTransform(0.0f,
                            -f_interwheel_distance * 0.5f,
                            m_fWheelRadius,
                            physx::PxQuat(CRadians::PI_OVER_TWO.GetValue(),
                                          physx::PxVec3(1.0f, 0.0f, 0.0f)));
      /* Create cylinder geometry for wheels */
      physx::PxGeometry* pcWheelGeometry =
         CreateCylinderGeometry(m_cPhysXEngine,
                                m_fWheelRadius,
                                m_fWheelThickness);
      /*
       * Create main body actor and shapes
       */
      /* Actor */
      m_pcMainBodyActor =
         m_cPhysXEngine.GetPhysics().createRigidDynamic(m_cMainBodyOffset);
      m_pcMainBodyActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, true);
      /* Shape */
      m_pcMainBodyShape =
         m_pcMainBodyActor->createShape(physx::PxBoxGeometry(c_body_size * 0.5f),
                                        m_cPhysXEngine.GetDefaultMaterial());
      /* Set inertial properties */
      physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pcMainBodyActor,
                                                     f_body_mass);
      /* Add actor to the scene */
      m_cPhysXEngine.GetScene().addActor(*m_pcMainBodyActor);
      /*
       * Create left wheel actor and shapes
       */
      /* Actor */
      m_pcLeftWheelActor =
         m_cPhysXEngine.GetPhysics().createRigidDynamic(m_cLeftWheelOffset);
      m_pcLeftWheelActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD,
                                           true);
      /* Shape */
      m_pcLeftWheelShape =
         m_pcLeftWheelActor->createShape(*pcWheelGeometry,
                                         m_cPhysXEngine.GetDefaultMaterial());
      /* Set inertial properties */
      physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pcLeftWheelActor,
                                                     f_wheel_mass);
      /* Add actor to the scene */
      m_cPhysXEngine.GetScene().addActor(*m_pcLeftWheelActor);
      /*
       * Create right wheel actor and shapes
       */
      /* Actor */
      m_pcRightWheelActor =
         m_cPhysXEngine.GetPhysics().createRigidDynamic(m_cRightWheelOffset);
      m_pcRightWheelActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD,
                                            true);
      /* Shape */
      m_pcRightWheelShape =
         m_pcRightWheelActor->createShape(*pcWheelGeometry,
                                          m_cPhysXEngine.GetDefaultMaterial());
      /* Set inertial properties */
      physx::PxRigidBodyExt::setMassAndUpdateInertia(*m_pcRightWheelActor,
                                                     f_wheel_mass);
      /* Add actor to the scene */
      m_cPhysXEngine.GetScene().addActor(*m_pcRightWheelActor);
      /*
       * Connect actors with joints
       *
       * The revolute joint locks all the degrees of freedom, apart from a single
       * rotational one, which works along the local X axis.
       * For this reason, one must orient the joint's X axis along the corresponding
       * axes of rotation of the objects involved (the wheels and the main body).
       *
       * The rotation axis of a joint corresponds to the local Z axis of a
       * wheel, and the origin of a joint corresponds to the origin of a wheel.
       */
      /*
       * Left wheel
       */
      /* Connect left wheel to main body */
      m_pcLeftWheelJoint = physx::PxRevoluteJointCreate(
         m_cPhysXEngine.GetPhysics(),
         m_pcMainBodyActor,
         physx::PxTransform(
            physx::PxVec3(m_cLeftWheelOffset.p.x,
                          m_cLeftWheelOffset.p.y,
                          m_cLeftWheelOffset.p.z - m_cMainBodyOffset.p.z),
            physx::PxQuat(CRadians::PI_OVER_TWO.GetValue(),
                          physx::PxVec3(0.0f, 0.0f, 1.0f))),
         m_pcLeftWheelActor,
         physx::PxTransform(
            physx::PxQuat(-CRadians::PI_OVER_TWO.GetValue(),
                          physx::PxVec3(0.0f, 1.0f, 0.0f))));
      /* Make this joint a motor */
      m_pcLeftWheelJoint->setRevoluteJointFlag(
         physx::PxRevoluteJointFlag::eDRIVE_ENABLED,
         true);
      /* Connect right wheel to main body */
      m_pcRightWheelJoint = physx::PxRevoluteJointCreate(
         m_cPhysXEngine.GetPhysics(),
         m_pcMainBodyActor,
         physx::PxTransform(
            physx::PxVec3(m_cRightWheelOffset.p.x,
                          m_cRightWheelOffset.p.y,
                          m_cRightWheelOffset.p.z - m_cMainBodyOffset.p.z),
            physx::PxQuat(-CRadians::PI_OVER_TWO.GetValue(),
                          physx::PxVec3(0.0f, 0.0f, 1.0f))),
         m_pcRightWheelActor,
         physx::PxTransform(
            physx::PxQuat(CRadians::PI_OVER_TWO.GetValue(),
                          physx::PxVec3(0.0f, 1.0f, 0.0f))));
      /* Make this joint a motor */
      m_pcRightWheelJoint->setRevoluteJointFlag(
         physx::PxRevoluteJointFlag::eDRIVE_ENABLED,
         true);
      /*
       * Add bodies to the owner model
       */
      c_model.AddBody(*m_pcMainBodyActor, m_cMainBodyOffset);
      c_model.AddBody(*m_pcLeftWheelActor, m_cLeftWheelOffset);
      c_model.AddBody(*m_pcRightWheelActor, m_cRightWheelOffset);
      /*
       * Cleanup
       */
      /* The wheel geometry is copied inside the wheel shapes, so the "original"
         geometry is not necessary anymore */
      delete pcWheelGeometry;
   }

   /****************************************/
   /****************************************/

   CPhysXDifferentialDrive::~CPhysXDifferentialDrive() {
      /* Remove the joints */
      m_pcLeftWheelJoint->release();
      m_pcRightWheelJoint->release();
      /* Remove the actors */
      m_pcMainBodyActor->release();
      m_pcLeftWheelActor->release();
      m_pcRightWheelActor->release();
   }

   /****************************************/
   /****************************************/

   void CPhysXDifferentialDrive::SetGlobalPose(const physx::PxTransform& c_pose) {
      /* Turn the dynamic actors into kinematic ones to allow for direct position control */
      m_pcMainBodyActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
      m_pcLeftWheelActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
      m_pcRightWheelActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
      /* Move the actors */
      m_pcMainBodyActor->setGlobalPose(c_pose * m_cMainBodyOffset);
      m_pcLeftWheelActor->setGlobalPose(c_pose * m_cLeftWheelOffset);
      m_pcRightWheelActor->setGlobalPose(c_pose * m_cRightWheelOffset);
      /* Turn the kinematic actors back into dynamic ones */
      m_pcMainBodyActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
      m_pcLeftWheelActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
      m_pcRightWheelActor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, false);
   }

   /****************************************/
   /****************************************/

}