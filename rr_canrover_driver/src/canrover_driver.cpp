#include "ros/ros.h"

#include <fcntl.h>
#include <termios.h>
#include <ctime>
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "std_msgs/Int32.h"
#include "std_msgs/Int32MultiArray.h"
#include "std_msgs/Float32MultiArray.h"
#include "geometry_msgs/Twist.h"
#include <std_msgs/Bool.h>
#include "nav_msgs/Odometry.h"

#include "canrover.hpp"
namespace canrover
{
  CanRover::CanRover(ros::NodeHandle &nh, ros::NodeHandle &nh_priv) : nh_(nh),
                                                                      nh_priv_(nh_priv),
                                                                      device_("can0"),
                                                                      e_stop_on_(false),
                                                                      LEFT_MOTOR_ID_(8),
                                                                      RIGHT_MOTOR_ID_(1)
  {
    ROS_INFO("Initializing Rover Can Driver");
  }

  bool CanRover::start()
  {
    
    if (!verifyParams())
    {
      ROS_WARN("Failed to setup ROBOT parameters.");
      return false;
    }

    ifname = device_.c_str();

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
    {
      ROS_ERROR("Error while opening socket");
      return -1;
    }

    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      ROS_ERROR("Error in socket bind");
      return -2;
    } 
    ROS_INFO("Successfuly setup robot parameters");
    cmd_vel_sub = nh_priv_.subscribe("/cmd_vel/managed", 1, &CanRover::cmdVelCB, this);
    e_stop_sub = nh_priv_.subscribe("/soft_estop/enable", 1, &CanRover::eStopCB, this);
    e_stop_reset_sub = nh_priv_.subscribe("/soft_estop/reset", 1, &CanRover::eStopResetCB, this);
    return true;
  }
  bool CanRover::verifyParams()
  {
    if (!(nh_priv_.getParam("device", device_)))
      ROS_WARN("Failed to retrieve can device name from parameter server.Defaulting to %s", device_.c_str());
    return true;
  }
  void CanRover::cmdVelCB(const geometry_msgs::Twist::ConstPtr &msg)
  {
    static bool prev_e_stop_state_ = false;
    double diff_vel_commanded = msg->angular.z;
    if (e_stop_on_)
    {
      if (!prev_e_stop_state_)
      {
        prev_e_stop_state_ = true;
        ROS_WARN("Rover driver - Soft e-stop on.");
      }
      CanSetDuty(LEFT_MOTOR_ID_, MOTOR_NEUTRAL);
      CanSetDuty(RIGHT_MOTOR_ID_, MOTOR_NEUTRAL);
      return;
    }
    else
    {
      if (prev_e_stop_state_)
      {
        prev_e_stop_state_ = false;
        ROS_INFO("Rover driver - Soft e-stop off.");
      }
      CanSetDuty(LEFT_MOTOR_ID_, msg->linear.x + 0.5 * msg->angular.z);
      CanSetDuty(RIGHT_MOTOR_ID_, msg->linear.x - 0.5 * msg->angular.z);
    }
    return;
  }
  int CanRover::CanSetDuty(int MotorID, float Duty)
  {
   // ROS_INFO("receiving DUTY %f" , Duty);
    Duty = clip(Duty, -0.5, 0.5);
   // ROS_INFO("trimmed DUTY %f", Duty);
    int32_t v = static_cast<int32_t>(Duty * 100000.0);
    frame.can_id = MotorID | 0x80000000U;
    frame.can_dlc = 4;
    frame.data[0] = static_cast<uint8_t>((static_cast<uint32_t>(v) >> 24) & 0xFF);
    frame.data[1] = static_cast<uint8_t>((static_cast<uint32_t>(v) >> 16) & 0xFF);
    frame.data[2] = static_cast<uint8_t>((static_cast<uint32_t>(v) >> 8) & 0xFF);
    frame.data[3] = static_cast<uint8_t>(static_cast<uint32_t>(v) & 0xFF);
    nbytes = write(s, &frame, sizeof(struct can_frame));

   // printf("Wrote %d bytes\n", nbytes);
  }
  float CanRover::clip(float n, float lower, float upper)
  {
    return std::max(lower, std::min(n, upper));
  }
  void CanRover::eStopCB(const std_msgs::Bool::ConstPtr &msg)
  {
    static bool prev_e_stop_state_ = false;

    // e-stop only trigger on the rising edge of the signal and only deactivates when reset
    if (msg->data && !prev_e_stop_state_)
    {
      e_stop_on_ = true;
    }

    prev_e_stop_state_ = msg->data;
    return;
  }
  void CanRover::eStopResetCB(const std_msgs::Bool::ConstPtr &msg)
  {
    if (msg->data)
    {
      e_stop_on_ = false;
    }
    return;
  }

} // namespace canrover

int main(int argc, char *argv[])
{ // Create ROS node
  ros::init(argc, argv, "rr_canrover_driver_node");

  ros::NodeHandle nh("");
  ros::NodeHandle nh_priv("~");
  canrover::CanRover canrover(nh, nh_priv);
  /*        if( !nh )
          {
                  ROS_FATAL( "Failed to initialize NodeHandle" );
                  ros::shutdown( );
                  return -1;
          }
          if( !nh_priv )
          {
                  ROS_FATAL( "Failed to initialize private NodeHandle" );
                  delete nh;
                  ros::shutdown( );
                  return -2;
          }
          if( !canrover )
          {
                  ROS_FATAL( "Failed to initialize driver" );
                  delete nh_priv;
                  delete nh;
                  ros::shutdown( );
                  return -3;
          }
  */
  if (!canrover.start())
  {
    ROS_FATAL("Failed to start the can driver");
    ros::requestShutdown();
  }
  ros::spin();
  ros::waitForShutdown();
  /*
          delete canrover;
          delete nh_priv;
          delete nh;
  */

  return 0;
}
