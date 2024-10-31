#include <rclcpp/rclcpp.hpp>
#include <format_data/msg/data.hpp>
#include <format_data/msg/registration.hpp>
#include <fstream>
#include <vector>
#include <string>

class Sensor : public rclcpp::Node
{
public:
  Sensor(int32_t sensor_id, bool status, const std::string &filename, const rclcpp::NodeOptions &options, std::string name)
      : Node("sensor_node_" + std::to_string(sensor_id), options), line_index_(0), status_(status), sensor_id_(sensor_id)
  {
    // Create publishers for 2 topics. 1. registration_status_topic; 2. health_data_topic
    registration_status_publisher_ = this->create_publisher<format_data::msg::Registration>("registration_status_topic", 10);
    health_data_publisher_ = this->create_publisher<format_data::msg::Data>("health_data_topic", 10);

    // Read the file contents
    read_file(filename, lines_);

    // Publish the registration_status_topic (sensor ID and status)
    status_timer_ = this->create_wall_timer(
        std::chrono::seconds(1),
        std::bind(&Sensor::publish_registration_status, this));

    // Publish the health_data_topic only if status is true
    if (status_)
    {
      timer_ = this->create_wall_timer(
          std::chrono::seconds(1),
          std::bind(&Sensor::publish_health_data, this));
    }

    highRisk0 = name + "_HighRisk0";
    highRisk1 = name + "_HighRisk1";
    midRisk0 = name + "_MidRisk0";
    midRisk1 = name + "_MidRisk1";
    lowRisk = name + "_LowRisk";

    declare_parameter(highRisk0.c_str(), std::vector<double>{0});
    declare_parameter(highRisk1.c_str(), std::vector<double>{0});
    declare_parameter(midRisk0.c_str(), std::vector<double>{0});
    declare_parameter(midRisk1.c_str(), std::vector<double>{0});
    declare_parameter(lowRisk.c_str(), std::vector<double>{0});

  }

  // function to read data from a file for publishing in health_data_topic
private:
  void read_file(const std::string &filename, std::vector<std::string> &lines)
  {
    std::ifstream file(filename);
    if (!file.is_open())
    {
      RCLCPP_ERROR(this->get_logger(), "Failed to open file: %s", filename.c_str());
      return;
    }

    std::string line;
    while (std::getline(file, line))
    {
      if (!line.empty())
      {
        lines.push_back(line);
      }
    }

    file.close();
  }

  // function to publish health_data_topic
  void publish_health_data()
  {
    if (status_) // Check if the status is true before publishing
    {
      if (line_index_ < lines_.size())
      {
        auto message = format_data::msg::Data();
        try
        {
          message.num = sensor_id_;
          message.data = lines_[line_index_++];
          RCLCPP_INFO_STREAM(this->get_logger(), "Publishing to health_data_topic: " << message.num << ", " << message.data);
          health_data_publisher_->publish(message);
        }
        catch (const std::invalid_argument &e)
        {
          RCLCPP_ERROR(this->get_logger(), "Invalid integer format in file for topic 1.");
          return;
        }
      }
    }
  }

  // function to publish registration_status_topic
  void publish_registration_status()
  {
    auto message = format_data::msg::Registration();
    message.num = sensor_id_;
    message.data = status_ ? "true" : "false";
    RCLCPP_INFO_STREAM(this->get_logger(), "Publishing registration status: " << sensor_id_ << ", " << (status_ ? "true" : "false"));
    registration_status_publisher_->publish(message);
  }

  std::vector<std::string> lines_;
  size_t line_index_;
  bool status_;
  int32_t sensor_id_;
  rclcpp::Publisher<format_data::msg::Data>::SharedPtr health_data_publisher_;
  rclcpp::Publisher<format_data::msg::Registration>::SharedPtr registration_status_publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;
  std::string highRisk0, highRisk1, midRisk0, midRisk1, lowRisk;
};

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);

  // initialize 2 publisher nodes
  // std::string file_path_1 = "/home/windsurff/ros0_ws/src/system/src/number.txt";
  std::string file_path_1 = "/home/ws/src/system/src/temperature_data.txt";
  bool status_1 = true;
  auto options_1 = rclcpp::NodeOptions().arguments({"--ros-args", "-r", "__node:=sensor_1"});
  int32_t sensor_id_1 = 1;
  auto node_1 = std::make_shared<Sensor>(sensor_id_1, status_1, file_path_1, options_1, "trm");

  // std::string file_path_2 = "/home/windsurff/ros0_ws/src/system/src/numberstring.txt";
  std::string file_path_2 = "/home/ws/src/system/src/heart_rate.txt";
  bool status_2 = true;
  auto options_2 = rclcpp::NodeOptions().arguments({"--ros-args", "-r", "__node:=sensor_2"});
  int32_t sensor_id_2 = 2;
  auto node_2 = std::make_shared<Sensor>(sensor_id_2, status_2, file_path_2, options_2, "hr");

  // Create an executor to manage both nodes
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node_1);
  executor.add_node(node_2);

  // Spin the executor to allow both nodes to run concurrently
  executor.spin();

  rclcpp::shutdown();
  return 0;
}
