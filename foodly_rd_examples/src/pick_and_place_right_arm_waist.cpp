// Copyright 2024 RT Corporation

#include <cmath>

#include "angles/angles.h"
#include "moveit/move_group_interface/move_group_interface.h"
#include "rclcpp/rclcpp.hpp"
#include "pose_presets.hpp"

using MoveGroupInterface = moveit::planning_interface::MoveGroupInterface;

static const rclcpp::Logger LOGGER = rclcpp::get_logger("pick_and_place_right_arm_waist");

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions node_options;
  node_options.automatically_declare_parameters_from_overrides(true);
  auto move_group_arm_node = rclcpp::Node::make_shared("move_group_arm_node", node_options);
  auto move_group_gripper_node = rclcpp::Node::make_shared("move_group_gripper_node", node_options);
  // For current state monitor
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(move_group_arm_node);
  executor.add_node(move_group_gripper_node);
  std::thread([&executor]() {executor.spin();}).detach();

  MoveGroupInterface move_group_arm(move_group_arm_node, "r_arm_waist_group");
  move_group_arm.setMaxVelocityScalingFactor(0.1);  // Set 0.0 ~ 1.0
  move_group_arm.setMaxAccelerationScalingFactor(0.1);  // Set 0.0 ~ 1.0

  MoveGroupInterface move_group_gripper(move_group_gripper_node, "r_gripper_group");
  move_group_gripper.setMaxVelocityScalingFactor(1.0);  // Set 0.0 ~ 1.0
  move_group_gripper.setMaxAccelerationScalingFactor(1.0);  // Set 0.0 ~ 1.0

  auto gripper_joint_values = move_group_gripper.getCurrentJointValues();
  const double GRIPPER_CLOSE = 0.0;
  const double GRIPPER_OPEN = angles::from_degrees(40.0);
  const double GRIPPER_GRASP = angles::from_degrees(20.0);

  const double PICK_POSITION_X = 0.25;
  const double PICK_POSITION_Y = 0.0;
  const double PICK_POSITION_Z = 0.12;

  const double PLACE_POSITION_X = 0.35;
  const double PLACE_POSITION_Y = 0.0;
  const double PLACE_POSITION_Z = 0.12;

  const double LIFTING_HEIGHT = 0.25;

  move_group_arm.setNamedTarget("r_arm_waist_init_pose");
  move_group_arm.move();

  gripper_joint_values[0] = GRIPPER_OPEN;
  move_group_gripper.setJointValueTarget(gripper_joint_values);
  move_group_gripper.move();

  // Set the range of motion of the waist
  moveit_msgs::msg::Constraints constraints;
  constraints.name = "arm_constraints";

  moveit_msgs::msg::JointConstraint joint_constraint;
  joint_constraint.joint_name = "waist_yaw_joint";
  joint_constraint.position = 0.0;
  joint_constraint.tolerance_above = angles::from_degrees(45);
  joint_constraint.tolerance_below = angles::from_degrees(45);
  joint_constraint.weight = 1.0;
  constraints.joint_constraints.push_back(joint_constraint);

  move_group_arm.setPathConstraints(constraints);

  // Picking
  move_group_arm.setPoseTarget(
    pose_presets::right_arm_downward(PICK_POSITION_X, PICK_POSITION_Y, LIFTING_HEIGHT));
  move_group_arm.move();

  move_group_arm.setPoseTarget(
    pose_presets::right_arm_downward(PICK_POSITION_X, PICK_POSITION_Y, PICK_POSITION_Z));
  move_group_arm.move();

  gripper_joint_values[0] = GRIPPER_GRASP;
  move_group_gripper.setJointValueTarget(gripper_joint_values);
  move_group_gripper.move();

  move_group_arm.setPoseTarget(
    pose_presets::right_arm_downward(PICK_POSITION_X, PICK_POSITION_Y, LIFTING_HEIGHT));
  move_group_arm.move();

  move_group_arm.setPoseTarget(
    pose_presets::right_arm_downward(PLACE_POSITION_X, PLACE_POSITION_Y, LIFTING_HEIGHT));
  move_group_arm.move();

  // Placing
  move_group_arm.setPoseTarget(
    pose_presets::right_arm_downward(PLACE_POSITION_X, PLACE_POSITION_Y, PLACE_POSITION_Z));
  move_group_arm.move();

  gripper_joint_values[0] = GRIPPER_OPEN;
  move_group_gripper.setJointValueTarget(gripper_joint_values);
  move_group_gripper.move();

  move_group_arm.setPoseTarget(
    pose_presets::right_arm_downward(PLACE_POSITION_X, PLACE_POSITION_Y, LIFTING_HEIGHT));
  move_group_arm.move();

  move_group_arm.clearPathConstraints();

  move_group_arm.setNamedTarget("r_arm_waist_init_pose");
  move_group_arm.move();

  gripper_joint_values[0] = GRIPPER_CLOSE;
  move_group_gripper.setJointValueTarget(gripper_joint_values);
  move_group_gripper.move();

  rclcpp::shutdown();
  return 0;
}
