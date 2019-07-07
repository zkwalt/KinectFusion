#pragma once

#include <memory>

#include <Eigen/Dense>
#include <Eigen/Core>
#include <Eigen/StdVector>

#include <ceres/ceres.h>
#include <sophus/se3.hpp>

#include "local_parameterization_se3.hpp"

#include "Frame.h"

class PointToPlaneConstraint {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    PointToPlaneConstraint(const Eigen::Vector3d& sourcePoint, const Eigen::Vector3d& targetPoint, const Eigen::Vector3d& targetNormal);

    template <typename T>
    bool operator()(T const* const sPose, T* sResiduals) const;

    static ceres::CostFunction* create(const Eigen::Vector3d& sourcePoint, const Eigen::Vector3d& targetPoint, const Eigen::Vector3d& targetNormal);

protected:
    Eigen::Vector3d m_source_point;
    Eigen::Vector3d m_target_point;
    Eigen::Vector3d m_target_normal;
};

class icp {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    icp(double dist_threshold, double normal_threshold);
    void estimatePose(std::shared_ptr<Frame> prev_frame, std::shared_ptr<Frame> current_frame, size_t m_nIterations,Eigen::Matrix4d& estimated_pose);

    const Eigen::Matrix4d getPose(Eigen::Matrix<double, 6, 1>& x);

private:

    double getb(Eigen::Vector3d& s_i, Eigen::Vector3d& n_i, Eigen::Vector3d& d_i);
    Eigen::Matrix<double, 6, 1> getA(Eigen::Vector3d& s_i, Eigen::Vector3d& n_i);

    Eigen::Matrix4d solveForPose(std::shared_ptr<Frame> prev_frame, std::shared_ptr<Frame> curr_frame,
            std::vector<std::pair<size_t,size_t>>& corresponding_points);

    bool hasValidDistance(const Eigen::Vector3d& point1, const Eigen::Vector3d& point2);
    bool hasValidAngle(const Eigen::Vector3d& normal1, const Eigen::Vector3d& normal2);

    void findCorrespondence(std::shared_ptr<Frame> prev_frame, std::shared_ptr<Frame> curr_frame,std::vector<std::pair<size_t,size_t>>& corresponding_points,Eigen::Matrix4d& estimated_pose);
    void prepareConstraints(std::shared_ptr<Frame> prev_frame, std::shared_ptr<Frame> curr_frame,std::vector<std::pair<size_t,size_t>>& corresponding_points, Sophus::SE3d& pose, ceres::Problem& problem);
    void configureSolver(ceres::Solver::Options& options);

    double dist_threshold;
    double normal_threshold;



    //Eigen::Matrix<double, Eigen::Dynamic, 6> m_A;
    //Eigen::Matrix<double, Eigen::Dynamic, 1> m_b;
    //Eigen::Vector3d m_x;
};