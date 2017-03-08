/*
 * ParticleFilter.cpp
 *
 *  Created on: Mar 21, 2012
 *      Author: nair
 */
#include "ParticleFilter.h"

#include <gsl/gsl_randist.h>

#include <QSettings>
#include <QFileInfo>


namespace tracking1
{

ParticleFilterParams::ParticleFilterParams()
{

}

ParticleFilterParams::~ParticleFilterParams()
{

}

void ParticleFilterParams::parse(std::string filename)
{
    QString name(filename.c_str());
    QFileInfo config(name);

    if(!config.exists())
    {
        std::cout<<"error reading config.ini file"<<std::endl;
        throw;
    }

    QSettings iniFile(name, QSettings::IniFormat);

    iniFile.beginGroup("PARTICLE_FILTER");
    NUMBER_OF_PARTICLES = iniFile.value("NUMBER_OF_PARTICLES", "100").toInt();

    TRANS_X_STD = iniFile.value("TRANS_X_STD", "100").toFloat();
    TRANS_Y_STD = iniFile.value("TRANS_Y_STD", "100").toFloat();
    TRANS_SCALE_STD = iniFile.value("TRANS_SCALE_STD", "100").toFloat();

    DYNAMICS_A1 = iniFile.value("DYNAMICS_A1", "100").toFloat();
    DYNAMICS_A2 = iniFile.value("DYNAMICS_A2", "100").toFloat();
    DYNAMICS_B0 = iniFile.value("DYNAMICS_B0", "100").toFloat();

    LAMBDA = iniFile.value("LAMBDA", "100").toDouble();

    DYNAMIC_MODEL_CV = iniFile.value("DYNAMIC_MODEL_CV", "false").toBool();

    std::cout << "PARTICLE_FILTER " << std::endl;
    std::cout << "NUMBER_OF_PARTICLES " << NUMBER_OF_PARTICLES << std::endl;
    std::cout << "TRANS_X_STD " << TRANS_X_STD << std::endl;
    std::cout << "TRANS_Y_STD " << TRANS_Y_STD << std::endl;
    std::cout << "TRANS_SCALE_STD " << TRANS_SCALE_STD << std::endl;
    std::cout << "DYNAMICS_A1 " << DYNAMICS_A1 << std::endl;
    std::cout << "DYNAMICS_A2 " << DYNAMICS_A2 << std::endl;
    std::cout << "DYNAMICS_B0 " << DYNAMICS_B0 << std::endl;
    std::cout << "LAMBDA      " << LAMBDA << std::endl;
    std::cout << "DYNAMIC_MODEL_CV " << (int)DYNAMIC_MODEL_CV << std::endl;

    iniFile.endGroup();
}


ParticleFilter::ParticleFilter()
{
    mParams.parse("config.ini");

    gsl_rng_env_setup();
    rng = gsl_rng_alloc( gsl_rng_mt19937 );
    gsl_rng_set( rng, time(NULL) );

    A_Matrix.resize(6,6);
    A_Matrix <<  1, 0, 0, 1, 0, 0,
            0, 1, 0, 0, 1, 0,
            0, 0, 1, 0, 0, 1,
            0, 0, 0, 1, 0, 0,
            0, 0, 0, 0, 1, 0,
            0, 0, 0, 0, 0, 1;

    G_Matrix.resize(6, 3);
    G_Matrix << 0.5, 0, 0,
            0, 0.5, 0,
            0, 0, 0.5,
            1, 0, 0,
            0, 1, 0,
            0, 0, 1;

    W_Matrix.resize(3,1);
    W_Matrix << 1, 1, 1;

    State_Matrix.resize(6,1);
    State_Prev_Matrix.resize(6,1);

}

ParticleFilter::~ParticleFilter() {
    delete[] mParticles;
}

void ParticleFilter::initialize(const cv::Vec3d& initPose)
{

    mParticles = new Particle[mParams.NUMBER_OF_PARTICLES];
    //malloc( mParams.NUMBER_OF_PARTICLES * sizeof( Particle ) );

    /* create particles at the centers of each of n regions */

    //std::cout << "initializing particles at " << initPose[0] << " " << initPose[1] << " " << initPose[2] << std::endl;
    for(int i=0; i<mParams.NUMBER_OF_PARTICLES ; ++i)
    {
        mParticles[i].x0 = mParticles[i].xp = mParticles[i].x = initPose[0];
        mParticles[i].y0 = mParticles[i].yp = mParticles[i].y = initPose[1];
        mParticles[i].s0 = mParticles[i].sp = mParticles[i].s = initPose[2];
        mParticles[i].x_velocity = 0.0f;
        mParticles[i].y_velocity = 0.0f;
        mParticles[i].s_velocity = 0.0f;
        mParticles[i].w  = 0.0f;
    }
}

void ParticleFilter::resizeParticleSet(int n)
{
    Particle* newParticles = new Particle[n];
    //malloc( mParams.NUMBER_OF_PARTICLES * sizeof( Particle ) );

    std::qsort( mParticles, mParams.NUMBER_OF_PARTICLES, sizeof( Particle ), particle_cmp );
    /* create particles at the centers of each of n regions */
    for(int i=0; i<n ; ++i)
    {
        newParticles[i].x0 = newParticles[i].xp = newParticles[i].x = mParticles[0].x;
        newParticles[i].y0 = newParticles[i].yp = newParticles[i].y = mParticles[0].y;
        newParticles[i].s0 = newParticles[i].sp = newParticles[i].s = mParticles[0].s;
        newParticles[i].x_velocity = mParticles[0].x_velocity;
        newParticles[i].y_velocity = mParticles[0].y_velocity;
        newParticles[i].s_velocity = mParticles[0].s_velocity;
        newParticles[i].w  = mParticles[0].w;
    }

    mParams.NUMBER_OF_PARTICLES = n;
    delete[] mParticles;
    mParticles = newParticles;
}


void ParticleFilter::predict()
{

    if(mParams.DYNAMIC_MODEL_CV)
    {
        predict_with_cv_model();

    }
    else
    {
        predict_with_simple_model();
    }


}

void ParticleFilter::predict_with_simple_model()
{
    float s;
    float TRANS_X_STD = mParams.TRANS_X_STD;
    float TRANS_Y_STD = mParams.TRANS_Y_STD;
    float TRANS_SCALE_STD = mParams.TRANS_SCALE_STD;
    Particle p;

    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        p.x = mParticles[i].x + gsl_ran_gaussian(rng, TRANS_X_STD);
        p.y = mParticles[i].y + gsl_ran_gaussian(rng, TRANS_Y_STD);
        s = mParticles[i].s + gsl_ran_gaussian(rng, TRANS_SCALE_STD);
        p.s = MAX(0.1, MIN(3.0, s));

        p.xp = mParticles[i].x;
        p.yp = mParticles[i].y;
        p.sp = mParticles[i].s;

        p.x0 = mParticles[i].x0;
        p.y0 = mParticles[i].y0;
        p.s0 = mParticles[i].s0;
        p.w = 0;

        mParticles[i].x = p.x;
        mParticles[i].y = p.y;
        mParticles[i].s = p.s;

        mParticles[i].xp = p.xp;
        mParticles[i].yp = p.yp;
        mParticles[i].sp = p.sp;

        mParticles[i].x0 = p.x0;
        mParticles[i].y0 = p.y0;
        mParticles[i].s0 = p.s0;

        mParticles[i].x_velocity = mParticles[i].x - mParticles[i].xp;
        mParticles[i].y_velocity = mParticles[i].y - mParticles[i].yp;
        mParticles[i].s_velocity = mParticles[i].s - mParticles[i].sp;
        mParticles[i].w = p.w;
    }
}

void ParticleFilter::predict_with_cv_model()
{
    static float TRANS_X_STD = mParams.TRANS_X_STD;
    static float TRANS_Y_STD = mParams.TRANS_Y_STD;
    static float TRANS_SCALE_STD = mParams.TRANS_SCALE_STD;

    static Particle p;
    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        W_Matrix(0, 0)=gsl_ran_gaussian( rng, TRANS_X_STD ) ;
        W_Matrix(1, 0)=gsl_ran_gaussian( rng, TRANS_Y_STD ) ;
        W_Matrix(2, 0)=gsl_ran_gaussian( rng, TRANS_SCALE_STD ) ;

        State_Prev_Matrix << mParticles[i].x ,
                mParticles[i].y,
                mParticles[i].s,
                mParticles[i].x_velocity ,
                mParticles[i].y_velocity,
                mParticles[i].s_velocity;


        State_Matrix = (A_Matrix*State_Prev_Matrix) + (G_Matrix*W_Matrix);


        p.x = State_Matrix(0, 0);
        p.y = State_Matrix(1, 0);
        p.s = State_Matrix(2, 0);
        p.x_velocity = State_Matrix(3, 0);
        p.y_velocity = State_Matrix(4, 0);
        p.s_velocity = State_Matrix(5, 0);

        p.xp = mParticles[i].x;
        p.yp = mParticles[i].y;
        p.sp = mParticles[i].s;

        p.x0 = mParticles[i].x0;
        p.y0 = mParticles[i].y0;
        p.s0 = mParticles[i].s0;
        p.w = 0;

        mParticles[i].x = p.x;
        mParticles[i].y = p.y;
        mParticles[i].s = p.s;

        mParticles[i].xp = p.xp;
        mParticles[i].yp = p.yp;
        mParticles[i].sp = p.sp;

        mParticles[i].x0 = p.x0;
        mParticles[i].y0 = p.y0;
        mParticles[i].s0 = p.s0;

        mParticles[i].x_velocity = p.x_velocity;
        mParticles[i].y_velocity = p.y_velocity;
        mParticles[i].s_velocity = p.s_velocity;
        mParticles[i].w = p.w;

    }
}

void ParticleFilter::correct()
{
    normalize();

    resample();

    //Op is the first particle
    //qsort( mParticles, mParams.NUMBER_OF_PARTICLES, sizeof( Particle ), particle_cmp);
}

void ParticleFilter::normalize()
{
    float sum = 0.0f;

    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
        sum += mParticles[i].w;
    if(sum > 0)
    {
        for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i )
            mParticles[i].w /= sum;
    }
    else
    {
        for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i )
            mParticles[i].w = 0.0f;
    }
}


void ParticleFilter::resample()
{

    Particle* new_particles;
    int np, k = 0;


    std::qsort( mParticles, mParams.NUMBER_OF_PARTICLES, sizeof( Particle ), particle_cmp );

    new_particles = new Particle[mParams.NUMBER_OF_PARTICLES];// malloc( mParams.NUMBER_OF_PARTICLES * sizeof( Particle ) );
    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        np = cvRound( mParticles[i].w * mParams.NUMBER_OF_PARTICLES);
        for(int j = 0; j<np; ++j)
        {
            new_particles[k++] = mParticles[i];
            if( k == mParams.NUMBER_OF_PARTICLES )
            {
                goto exit;

            }
        }
    }
    while( k < mParams.NUMBER_OF_PARTICLES )
        new_particles[k++] = mParticles[0];

exit:
    delete[] mParticles;
    mParticles = new_particles;
}

void ParticleFilter::setParticleWeight(int id, float w)
{
    mParticles[id].w = w;
}

void ParticleFilter::getOutputPose(cv::Vec3d& pose)
{
    pose[0] = 0.0;//mParticles[0].x;
    pose[1] = 0.0;//mParticles[0].y;
    pose[2] = 0.0;//mParticles[0].s;


    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        pose[0]+=mParticles[i].x;
        pose[1]+=mParticles[i].y;
        pose[2]+=mParticles[i].s;
    }

    if(mParams.NUMBER_OF_PARTICLES>0)
    {
        pose[0] /=mParams.NUMBER_OF_PARTICLES;
        pose[1] /=mParams.NUMBER_OF_PARTICLES;
        pose[2] /=mParams.NUMBER_OF_PARTICLES;
    }
    else
    {
        pose[0] = 0.0;//mParticles[0].x;
        pose[1] = 0.0;//mParticles[0].y;
        pose[2] = 0.0;//mParticles[0].s;
    }

    //std::cout << "Output pose " << pose[0] << " " << pose[1] << " " << pose[2] << std::endl;
}


void ParticleFilter::getOutputAvgPose(cv::Vec3d& pose)
{
    pose[0] = 0.0;//mParticles[0].x;
    pose[1] = 0.0;//mParticles[0].y;
    pose[2] = 0.0;//mParticles[0].s;


    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        pose[0]+=mParticles[i].x;
        pose[1]+=mParticles[i].y;
        pose[2]+=mParticles[i].s;
    }

    if(mParams.NUMBER_OF_PARTICLES>0)
    {
        pose[0] /=mParams.NUMBER_OF_PARTICLES;
        pose[1] /=mParams.NUMBER_OF_PARTICLES;
        pose[2] /=mParams.NUMBER_OF_PARTICLES;
    }
    else
    {
        pose[0] = 0.0;//mParticles[0].x;
        pose[1] = 0.0;//mParticles[0].y;
        pose[2] = 0.0;//mParticles[0].s;
    }

    //std::cout << "Output pose " << pose[0] << " " << pose[1] << " " << pose[2] << std::endl;
}

void ParticleFilter::getParticlePose(int id, cv::Vec3d& pose)
{

    pose[0] = mParticles[id].x;
    pose[1] = mParticles[id].y;
    pose[2] = mParticles[id].s;

}

void ParticleFilter::getOutputVelocity(cv::Vec3d& vel)
{
    vel[0] = mParticles[0].x;
    vel[1] = mParticles[0].y;
    vel[2] = mParticles[0].s;
}

}

