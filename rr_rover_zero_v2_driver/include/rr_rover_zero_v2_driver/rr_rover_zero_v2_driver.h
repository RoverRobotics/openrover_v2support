// -*- mode:c++; fill-column: 100; -*-

#ifndef rr_rover_zero_v2_driver_rr_rover_zero_v2_driver_H_
#define rr_rover_zero_v2_driver_rr_rover_zero_v2_driver_H_

#include <string>

#include <ros/ros.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Bool.h>
#include <boost/optional.hpp>
#include "nav_msgs/Odometry.h"
#include "geometry_msgs/Twist.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include <tf2_ros/transform_broadcaster.h>
#include <geometry_msgs/TransformStamped.h>
#include "rr_rover_zero_v2_driver/vesc_interface.h"
#include "rr_rover_zero_v2_driver/vesc_packet.h"

namespace rr_rover_zero_v2_driver
{

  class Rr_Rover_ZERO_V2_Driver
  {
  public:
    Rr_Rover_ZERO_V2_Driver(ros::NodeHandle nh,
                            ros::NodeHandle private_nh);
    bool e_stop_on_;
    float trim;
    double odom_angular_coef_ ;
    double odom_traction_factor_ ;
    double hall_ratio_;

  private:
    // interface to the VESC
    VescInterface vesc_;
    void vescPacketCallback(const boost::shared_ptr<VescPacket const> &packet);
    void vescErrorCallback(const std::string &error);

    // ROS services
    ros::Publisher state_pub_;
    ros::Publisher odom_enc_pub_;
    ros::Subscriber twist_sub_;
    ros::Subscriber trim_sub_;
    ros::Subscriber e_stop_sub;
    ros::Subscriber e_stop_reset_sub;
    ros::Timer timer_;
    
    // driver modes (possible states)
    typedef enum
    {
      MODE_INITIALIZING,
      MODE_OPERATING
    } driver_mode_t;

    // other variables
    driver_mode_t driver_mode_; ///< driver state machine mode (state)
    int fw_version_major_;      ///< firmware major version reported by vesc
    int fw_version_minor_;      ///< firmware minor version reported by vesc
    int motorID_ = 0;
    double rpm_1 = 0;
    double rpm_2 = 0;
    
    // ROS callbacks
    void timerCallback(const ros::TimerEvent &event);
    void publishOdometry(float left_vel, float right_vel);
    void trimCB(const std_msgs::Float32::ConstPtr &msg);
    void callbackTwist(const geometry_msgs::Twist &msg);
    void eStopCB(const std_msgs::Bool::ConstPtr &msg);
    void eStopResetCB(const std_msgs::Bool::ConstPtr &msg);
    float clip(float n, float lower, float upper);
  };

} // namespace rr_rover_zero_v2_driver

#endif // rr_rover_zero_v2_driver_rr_rover_zero_v2_driver_H_
