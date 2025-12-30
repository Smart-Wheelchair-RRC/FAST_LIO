#include <iostream>
#include <filesystem>
#include <fstream>
#include <deque>
#include <vector>
#include <string>
#include <limits>
#include <cmath>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialization.hpp>
#include <rosbag2_cpp/reader.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/imu.hpp>


class BagProcessing : public rclcpp::Node {
    public: 
        BagProcessing(const std::string& input_path)
        : Node("bag_processing"), input_path_(input_path) {

            imu_pub_ = this->create_publisher<sensor_msgs::msg::Imu>("/livox/imu", 10);
            lidar_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("/livox/lidar", 10);

            playback_thread_ = std::thread(&BagProcessing::ros_play, this);
        }

        ~BagProcessing() {
            if (playback_thread_.joinable()) {
                playback_thread_.join();
            }
        }
    
    private:
        std::string input_path_;
        rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
        rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr lidar_pub_;
        std::thread playback_thread_;

        void ros_play() {
            rosbag2_storage::StorageOptions storage_options;
            storage_options.uri = input_path_;
            storage_options.storage_id = "sqlite3"; 

            rosbag2_cpp::ConverterOptions converter_options;
            converter_options.input_serialization_format = "cdr";
            converter_options.output_serialization_format = "cdr";

            rosbag2_cpp::Reader reader;

            reader.open(storage_options, converter_options);

            rclcpp::Serialization<sensor_msgs::msg::Imu> imu_ser;
            rclcpp::Serialization<sensor_msgs::msg::PointCloud2> pcl_ser;

            rcutils_time_point_value_t first_bag_time = 0;
            std::chrono::steady_clock::time_point start_wall_time;
            bool is_first_msg = true;

            while (reader.has_next() && rclcpp::ok()) {

                auto bag_message = reader.read_next();
                std::string topic = bag_message->topic_name;
                rcutils_time_point_value_t msg_timestamp = bag_message->time_stamp;

                if(is_first_msg == true) {
                    first_bag_time = msg_timestamp;
                    start_wall_time = std::chrono::steady_clock::now();
                    is_first_msg = false;
                } else {
                    auto bag_elapsed = std::chrono::nanoseconds(msg_timestamp - first_bag_time);
                    auto target_wall_time = start_wall_time + bag_elapsed;
                    
                    std::this_thread::sleep_until(target_wall_time);
                }

                rclcpp::SerializedMessage serialized_msg(*bag_message->serialized_data);

                if (topic == "/livox/imu") {
                    sensor_msgs::msg::Imu msg;
                    imu_ser.deserialize_message(&serialized_msg, &msg);
                    imu_pub_->publish(msg);
                }
                else if (topic == "/livox/lidar") {
                    sensor_msgs::msg::PointCloud2 msg;
                    pcl_ser.deserialize_message(&serialized_msg, &msg);
                    lidar_pub_->publish(msg);
                }
            }
            rclcpp::shutdown();
        }
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);

    std::string bag_path = "/home/container_user/husky_data/src/records/1/rosbag_0.db3";
    
    auto non_ros_args = rclcpp::remove_ros_arguments(argc, argv);

    if (non_ros_args.size() > 1) {
        bag_path = non_ros_args[1];
    } else {
        std::cout << "No bag path provided. Using default: " << bag_path << std::endl;
    }

    auto node = std::make_shared<BagProcessing>(bag_path);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}