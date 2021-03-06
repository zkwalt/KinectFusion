
#include <librealsense2/rs.h>
#include <librealsense2/rs.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <zconf.h>
#include <Volume.hpp>
#include <Fusion.hpp>
#include <Raycast.hpp>
#include <Recorder.h>
#include <MeshWriter.h>
#include <KinectVirtualSensor.h>

#include "VirtualSensor.h"
#include "icp.h"
#include "Frame.h"
#include "Marching_cubes.hpp"
#include <data_types.h>
Fusion fusion;
Raycast raycast;
VirtualSensor sensor;
// KinectVirtualSensor sensor(PROJECT_DATA_DIR + std::string("/sample0"), 5 );
//Recorder rec;

bool process_frame( size_t frame_cnt, std::shared_ptr<Frame> prevFrame,std::shared_ptr<Frame> currentFrame, std::shared_ptr<Volume> volume,const Config& config)
{
    // STEP 1: estimate Pose
    icp icp(config.m_dist_threshold,config.m_normal_threshold);

    Eigen::Matrix4d estimated_pose = prevFrame->getGlobalPose();
    currentFrame->setGlobalPose(estimated_pose);

    std::cout << "Init: ICP..." << std::endl;
    if(!icp.estimatePose(frame_cnt, prevFrame,currentFrame, 3, estimated_pose)){
        throw "ICP Pose Estimation failed";
    };

    if ((frame_cnt-1) % 5 == 0) {
        std::stringstream filename;
        filename << "frame" << frame_cnt << "pre";
        MeshWriter::toFile(filename.str(), currentFrame);
    }

    // STEP 2: Surface reconstruction
    std::cout << "Init: Fusion..." << std::endl;
    if(!fusion.reconstructSurface(currentFrame,volume, config.m_truncationDistance)){
        throw "Surface reconstruction failed";
    };

    std::cout << "Init: Raycast..." << std::endl;
    if(!raycast.surfacePrediction(currentFrame,volume, config.m_truncationDistance)){
        throw "Raycasting failed";
    };
    std::cout << "Done!" << std::endl;
    return true;
}

int main(){

    // Recorder rec;
    // rec.record(10);


    // std::string filenameIn = PROJECT_DATA_DIR + std::string("/rgbd_dataset_freiburg1_xyz/");
    std::string filenameIn = PROJECT_DATA_DIR + std::string("/recording/");

    std::cout << "Initialize virtual sensor..." << std::endl;

    //KinectVirtualSensor sensor;
    VirtualSensor sensor;
    if (!sensor.init(filenameIn)) {
        std::cout << "Failed to initialize the sensor!\nCheck file path!" << std::endl;
        return -1;
    }



    /*
     * Configuration Stuff
     */
    Eigen::Vector3d volumeRange(5.0, 5.0, 5.0);
    Eigen::Vector3i volumeSize (512,512,512);
    double voxelSize = volumeRange.x()/volumeSize.x();

    const auto volumeOrigin = Eigen::Vector3d (-volumeRange.x()/2,-volumeRange.y()/2,0.5);

    Config config (0.1,0.5,0.06, volumeOrigin, volumeSize.x(),volumeSize.y(),volumeSize.z(), voxelSize);

    //print Configuration to File
    config.printToFile("config");

    /*
     * Setting up the Volume from Configuration
     */
    auto volume = std::make_shared<Volume>(config.m_volumeOrigin, config.m_volumeSize,config.m_voxelScale) ;

    /*
     * Process a first frame as a reference frame.
     * --> All next frames are tracked relatively to the first frame.
     */
    sensor.processNextFrame();
    Eigen::Matrix3d depthIntrinsics = sensor.getDepthIntrinsics();
    Eigen::Matrix3d colIntrinsics   = sensor.getColorIntrinsics();
    Eigen::Matrix4d d2cExtrinsics   = sensor.getD2CExtrinsics();
    const unsigned int depthWidth         = sensor.getDepthImageWidth();
    const unsigned int depthHeight        = sensor.getDepthImageHeight();

    const double* depthMap = &sensor.getDepth()[0];
    BYTE* colors = &sensor.getColorRGBX()[0];

    std::shared_ptr<Frame> prevFrame = std::make_shared<Frame>(Frame(depthMap, colors, depthIntrinsics, colIntrinsics, d2cExtrinsics, depthWidth, depthHeight));
    MeshWriter::toFile("mesh0", prevFrame);

    int i = 1;
    const int iMax = 20;

    while( i <= iMax && sensor.processNextFrame() ){

        const double* depthMap = &sensor.getDepth()[0];
        BYTE* colors = &sensor.getColorRGBX()[0];
        std::shared_ptr<Frame> currentFrame = std::make_shared<Frame>(Frame(depthMap, colors, depthIntrinsics,colIntrinsics, d2cExtrinsics, depthWidth, depthHeight));

        process_frame(i,prevFrame,currentFrame,volume,config);

        if ((i-1) % 5 == 0) {
            std::stringstream filename;
            filename << "frame" << i;
            MeshWriter::toFile(filename.str(), currentFrame);
            //Write Fused Volume to File with Marching Cubes Algorithm
            MeshWriter::toFileMarchingCubes( std::string("marchingCubes_") + std::to_string(i),*volume);
            //Write Fused Volume to File with Blocks indicating the Distance of each Voxel
            MeshWriter::toFileTSDF(std::string("tsdf_") + std::to_string(i),*volume);
        }

        prevFrame = std::move(currentFrame);
        i++;

    }
}
