/*
 * ParticleFilter.h
 *
 *  Created on: Mar 21, 2012
 *      Author: nair
 */

#ifndef PARTICLEFILTER_H_
#define PARTICLEFILTER_H_

#include <gsl/gsl_rng.h>
#include <opencv2/opencv.hpp>
#include <Eigen/Dense>


namespace tracking1
{

class ParticleFilterParams
{
public:
    ParticleFilterParams();
    ~ParticleFilterParams();

    void parse(std::string filename);

public:

    int NUMBER_OF_PARTICLES;

    /* standard deviations for gaussian sampling in transition model */
    float TRANS_X_STD;
    float TRANS_Y_STD;
    float TRANS_SCALE_STD;

    /* autoregressive dynamics parameters for transition model */
    float DYNAMICS_A1;
    float DYNAMICS_A2;
    float DYNAMICS_B0;

    double LAMBDA;

    bool DYNAMIC_MODEL_CV;
};

struct Particle
{
    float x;          /**< current x coordinate */
    float y;          /**< current y coordinate */
    float s;          /**< current z coordinate */
    float xp;         /**< previous x coordinate */
    float yp;         /**< previous y coordinate */
    float sp;         /**< previous z coordinate */
    float x0;         /**< original x coordinate */
    float y0;         /**< original y coordinate */
    float s0;			/**< original z coordinate */
    float x_velocity; /**< current x_velocity coordinate */
    float y_velocity; /**< current y_velocity coordinate */
    float s_velocity; /**< current z_velocity coordinate */

    float w;          /**< weight */
};

class ParticleFilter
{
public:

    ParticleFilter();
    ~ParticleFilter();

    void initialize(const cv::Vec3d& initPose);

    void predict();

    void predict_with_simple_model();

    void predict_with_cv_model();

    void correct();

    void resizeParticleSet(int n);
    void setParticleWeight(int id, float w);

    double getMaxLikelihood()
    {
        double max=0.0;
        for(int i=0;i<mParams.NUMBER_OF_PARTICLES;++i)
        {
            if(max<mParticles[i].w)
            {
                max=mParticles[i].w;
            }
        }
        return max;
    }

    double getAvgLikelihood()
    {
        double avg=0.0;
        for(int i=0;i<mParams.NUMBER_OF_PARTICLES;++i)
        {

            avg+=mParticles[i].w;
        }
        return avg/mParams.NUMBER_OF_PARTICLES;
    }

    int nOfParticles()
    {
        return mParams.NUMBER_OF_PARTICLES;
    }

    static int particle_cmp(const void* p1,const void* p2 )
    {
        Particle* _p1 = (Particle*)p1;
        Particle* _p2 = (Particle*)p2;

        if( _p1->w > _p2->w )
            return -1;
        if( _p1->w < _p2->w )
            return 1;
        return 0;
    }

    void getParticlePose(int id, cv::Vec3d& pose);

    void getOutputPose(cv::Vec3d& pose);

    void getOutputAvgPose(cv::Vec3d& pose);

    void getOutputVelocity(cv::Vec3d& vel);

    double getLambda()
    {
        return mParams.LAMBDA;
    }

private:

    void normalize();

    void resample();

private:


    ParticleFilterParams mParams;

    Particle* mParticles;
    cv::Vec3d mOutputPose;

    Eigen::MatrixXd A_Matrix;
    Eigen::MatrixXd G_Matrix;
    Eigen::MatrixXd W_Matrix;

    Eigen::MatrixXd State_Matrix;
    Eigen::MatrixXd State_Prev_Matrix;

    gsl_rng* rng;

};


}
#endif /* PARTICLEFILTER_H_ */
