#include <ros/ros.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Twist.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <std_msgs/Int32.h>
#include <cmath>
#include <tf/tf.h>
#include <geometry_msgs/Point.h>
#include <queue>
#define gravity 9.806

using namespace std;


mavros_msgs::State current_state;
geometry_msgs::TwistStamped desired_vel;   

void state_cb(const mavros_msgs::State::ConstPtr& msg) {
    current_state = *msg;
}


int main(int argc, char **argv)
{
    //  ROS_initialize  //
    ros::init(argc, argv, "velocity_cbf");
    ros::NodeHandle nh,private_nh("~");
    // Publshier //
    ros::Publisher local_vel_pub = nh.advertise<geometry_msgs::TwistStamped>("mavros/setpoint_velocity/cmd_vel", 2);
    //    subscriber    //
    ros::Subscriber state_sub = nh.subscribe<mavros_msgs::State>("mavros/state", 100, state_cb);
    // service
    ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool>("mavros/cmd/arming");
    ros::ServiceClient set_mode_client = nh.serviceClient<mavros_msgs::SetMode>("mavros/set_mode");
    ros::Rate rate(100);

    
    ROS_INFO("Wait for FCU connection");
    while (ros::ok() && !current_state.connected) {
        ros::spinOnce();
        rate.sleep();
        ROS_INFO("Wait for FCU");
    }
    ROS_INFO("FCU connected");

    
    //send a few velocity setpoints before starting
    for(int i = 0; ros::ok() && i < 20; i++){
        local_vel_pub.publish(desired_vel);
        std::cout << "Pub useless points\n";
        rate.sleep();
    }

    mavros_msgs::SetMode offb_set_mode;
    offb_set_mode.request.base_mode = 0 ; 
    offb_set_mode.request.custom_mode = "GUIDED";

    mavros_msgs::CommandBool arm_cmd;
    arm_cmd.request.value = true;
    ros::Time last_request = ros::Time::now();
    
    if( set_mode_client.call(offb_set_mode) && offb_set_mode.response.mode_sent) {
        ROS_INFO("GUIDED enabled");
    }



    if( arming_client.call(arm_cmd) && arm_cmd.response.success) {
        ROS_INFO("Vehicle armed");
    }


    ros::ServiceClient takeoff_cl = nh.serviceClient<mavros_msgs::CommandTOL>("mavros/cmd/takeoff");
    mavros_msgs::CommandTOL srv_takeoff;
    srv_takeoff.request.latitude = 0;
    srv_takeoff.request.longitude = 0;
    srv_takeoff.request.min_pitch = 0;
    srv_takeoff.request.yaw = 0;
    srv_takeoff.request.altitude = 1.2;


    sleep(5);

    if(takeoff_cl.call(srv_takeoff))
    {
        ROS_INFO("srv_takeoff send success %d", srv_takeoff.response.success);
        ROS_INFO("takeoff result %d", srv_takeoff.response.result);
    }
    else
    {
        ROS_ERROR("Takeoff failed");
	return 0;
    }


	
    sleep(10);


    ROS_INFO("get UAV all start signal");

    while (ros::ok()) {
    
        // //keyboard control
        // if(kill_all_drone == 1){
        //     ROS_WARN("velocity_cbf_kill!");
        //     offb_set_mode.request.custom_mode = "LOITER";
        //     set_mode_client.call(offb_set_mode);
        // }

        ros::spinOnce();
        rate.sleep();
    }
    return 0;
}


