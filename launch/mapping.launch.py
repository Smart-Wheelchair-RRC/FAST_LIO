import os.path
from pathlib import Path

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def setup_nodes(context, *args, **kwargs):
    package_path = Path(get_package_share_directory('fast_lio'))
    default_rviz_config = str(package_path / 'rviz' / 'fastlio.rviz')

    # Resolve arguments
    use_sim_time   = LaunchConfiguration('use_sim_time').perform(context)
    config_path    = LaunchConfiguration('config_path').perform(context)
    config_file    = LaunchConfiguration('config_file').perform(context)
    rviz_use       = LaunchConfiguration('rviz').perform(context)
    rviz_cfg       = LaunchConfiguration('rviz_cfg').perform(context)
    lid_topic      = LaunchConfiguration('lid_topic').perform(context)
    dense_publish  = LaunchConfiguration('dense_publish_en').perform(context)
    map_file_path  = LaunchConfiguration('map_file_path').perform(context)

    # Build parameter overrides (applied on top of the YAML config)
    overrides = {'use_sim_time': use_sim_time.lower() == 'true'}

    if lid_topic:
        overrides['common.lid_topic'] = lid_topic
    if dense_publish:
        overrides['publish.dense_publish_en'] = dense_publish.lower() == 'true'
    if map_file_path:
        overrides['map_file_path'] = map_file_path
        # Ensure the output directory exists
        output_dir = Path(map_file_path).parent
        output_dir.mkdir(parents=True, exist_ok=True)

    config_full = os.path.join(config_path, config_file)

    fast_lio_node = Node(
        package='fast_lio',
        executable='fastlio_mapping',
        parameters=[config_full, overrides],
        output='screen',
    )

    nodes = [fast_lio_node]

    if rviz_use.lower() == 'true':
        rviz_node = Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', rviz_cfg],
        )
        nodes.append(rviz_node)

    return nodes


def generate_launch_description():
    package_path = get_package_share_directory('fast_lio')
    default_config_path = os.path.join(package_path, 'config')
    default_rviz_config_path = os.path.join(
        package_path, 'rviz', 'fastlio.rviz')

    return LaunchDescription([
        DeclareLaunchArgument(
            'use_sim_time', default_value='false',
            description='Use simulation (Gazebo) clock if true'
        ),
        DeclareLaunchArgument(
            'config_path', default_value=default_config_path,
            description='Yaml config file path'
        ),
        DeclareLaunchArgument(
            'config_file', default_value='mid360.yaml',
            description='Config file'
        ),
        DeclareLaunchArgument(
            'rviz', default_value='true',
            description='Use RViz to monitor results'
        ),
        DeclareLaunchArgument(
            'rviz_cfg', default_value=default_rviz_config_path,
            description='RViz config file path'
        ),
        # -- Override arguments (empty = use YAML defaults) -----------------
        DeclareLaunchArgument(
            'lid_topic', default_value='',
            description='Override lidar topic (e.g. /livox/lidar_cropped)'
        ),
        DeclareLaunchArgument(
            'dense_publish_en', default_value='',
            description='Override dense publish (true/false)'
        ),
        DeclareLaunchArgument(
            'map_file_path', default_value='',
            description='Override PCD map save path'
        ),
        OpaqueFunction(function=setup_nodes),
    ])
