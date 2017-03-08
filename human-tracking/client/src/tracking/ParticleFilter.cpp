/*
 * ParticleFilter.cpp
 *
 *  Created on: Mar 21, 2012
 *      Author: nair
 */
#include "ParticleFilter.h"


namespace tracking
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
    TRANS_Z_STD = iniFile.value("TRANS_Z_STD", "100").toFloat();

    DYNAMICS_A1 = iniFile.value("DYNAMICS_A1", "100").toFloat();
    DYNAMICS_A2 = iniFile.value("DYNAMICS_A2", "100").toFloat();
    DYNAMICS_B0 = iniFile.value("DYNAMICS_B0", "100").toFloat();

    DYNAMIC_MODEL_CV = iniFile.value("DYNAMIC_MODEL_CV", "false").toBool();
    LAMBDA = iniFile.value("LAMBDA", "20.0").toFloat();

    std::cout << "PARTICLE_FILTER " << std::endl;
    std::cout << "NUMBER_OF_PARTICLES " << NUMBER_OF_PARTICLES << std::endl;
    std::cout << "TRANS_X_STD " << TRANS_X_STD << std::endl;
    std::cout << "TRANS_Y_STD " << TRANS_Y_STD << std::endl;
    std::cout << "TRANS_Z_STD " << TRANS_Z_STD << std::endl;
    std::cout << "DYNAMICS_A1 " << DYNAMICS_A1 << std::endl;
    std::cout << "DYNAMICS_A2 " << DYNAMICS_A2 << std::endl;
    std::cout << "DYNAMICS_B0 " << DYNAMICS_B0 << std::endl;
    std::cout << "DYNAMIC_MODEL_CV " << (int)DYNAMIC_MODEL_CV << std::endl;
    std::cout << "LAMBDA       " << LAMBDA << std::endl;


    iniFile.endGroup();

}


ParticleFilter::ParticleFilter(uint &filter_id)
  : filterId(filter_id)
  , mMaxLikelihood(0.0f)
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

ParticleFilter::~ParticleFilter()
{

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
        mParticles[i].z0 = mParticles[i].zp = mParticles[i].z = initPose[2];
        mParticles[i].x_velocity = 0.0f;
        mParticles[i].y_velocity = 0.0f;
        mParticles[i].z_velocity = 0.0f;
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
        newParticles[i].z0 = newParticles[i].zp = newParticles[i].z = mParticles[0].z;
        newParticles[i].x_velocity = mParticles[0].x_velocity;
        newParticles[i].y_velocity = mParticles[0].y_velocity;
        newParticles[i].z_velocity = mParticles[0].z_velocity;
        newParticles[i].w  = mParticles[0].w;
    }

    mParams.NUMBER_OF_PARTICLES = n;

    delete[] mParticles;

    mParticles = newParticles;
}

void ParticleFilter::setParticleWeightFromDistanceMeasure(int id, float dist)
{
    float likelihood=1.0;
    likelihood=exp(-0.5*dist*dist*mParams.LAMBDA);

    //std::cout << "likelihood " << id << " " << likelihood <<std::endl;
    setParticleWeight(id, likelihood);
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
    static float x, y, z;
    static float TRANS_X_STD = mParams.TRANS_X_STD;
    static float TRANS_Y_STD = mParams.TRANS_Y_STD;
    static float A1 = mParams.DYNAMICS_A1;
    static float A2 = mParams.DYNAMICS_A2;
    static float B0 = mParams.DYNAMICS_B0;

    static Particle p;
    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        x = A1 * ( mParticles[i].x -  mParticles[i].x0 ) + A2 * ( mParticles[i].xp - mParticles[i].x0 ) +
                B0 * gsl_ran_gaussian( rng, TRANS_X_STD ) + mParticles[i].x0;
        p.x = x;//MAX( 0.0, MIN( (float)4000.0 - 1.0, x ) );
        y = A1 * ( mParticles[i].y - mParticles[i].y0 ) + A2 * ( mParticles[i].yp - mParticles[i].y0 ) +
                B0 * gsl_ran_gaussian( rng, TRANS_Y_STD ) + mParticles[i].y0;
        p.y = y;//MAX( 0.0, MIN( (float)4000.0 - 1.0, y ) );
        z = mParticles[i].z;//A1 * ( mParticles[i].z - mParticles[i].z0 ) + A2 * ( mParticles[i].zp - mParticles[i].z0  ) +
        //B0 * gsl_ran_gaussian( rng, TRANS_Z_STD ) + mParticles[i].z0;
        p.z = z;//MAX( 0.0, MIN( (float)w - 1.0, z ) );

        p.xp = mParticles[i].x;
        p.yp = mParticles[i].y;
        p.zp = mParticles[i].z;

        p.x0 = mParticles[i].x0;
        p.y0 = mParticles[i].y0;
        p.z0 = mParticles[i].z0;
        p.w = 0;

        mParticles[i].x = p.x;
        mParticles[i].y = p.y;
        mParticles[i].z = p.z;

        mParticles[i].xp = p.xp;
        mParticles[i].yp = p.yp;
        mParticles[i].zp = p.zp;

        mParticles[i].x0 = p.x0;
        mParticles[i].y0 = p.y0;
        mParticles[i].z0 = p.z0;

        mParticles[i].x_velocity = mParticles[i].x - mParticles[i].xp;
        mParticles[i].y_velocity = mParticles[i].y - mParticles[i].yp;
        mParticles[i].z_velocity = mParticles[i].z - mParticles[i].zp;
        mParticles[i].w = p.w;

        //std::cout << "particle " << i << " " << mParticles[i].x << " " << mParticles[i].y << " " << mParticles[i].z << std::endl;

    }
}

void ParticleFilter::predict_with_cv_model()
{
    static float TRANS_X_STD = mParams.TRANS_X_STD;
    static float TRANS_Y_STD = mParams.TRANS_Y_STD;
    static float TRANS_Z_STD = mParams.TRANS_Z_STD;

    static Particle p;
    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        W_Matrix(0, 0)=gsl_ran_gaussian( rng, TRANS_X_STD ) ;
        W_Matrix(1, 0)=gsl_ran_gaussian( rng, TRANS_Y_STD ) ;
        W_Matrix(2, 0)=gsl_ran_gaussian( rng, TRANS_Z_STD ) ;

        State_Prev_Matrix << mParticles[i].x ,
                mParticles[i].y,
                mParticles[i].z,
                mParticles[i].x_velocity ,
                mParticles[i].y_velocity,
                mParticles[i].z_velocity;


        State_Matrix = (A_Matrix*State_Prev_Matrix) + (G_Matrix*W_Matrix);


        p.x = State_Matrix(0, 0);
        p.y = State_Matrix(1, 0);
        p.z = mParticles[i].z;//State_Matrix(2, 0);
        p.x_velocity = State_Matrix(3, 0);
        p.y_velocity = State_Matrix(4, 0);
        p.z_velocity = mParticles[i].z_velocity;//State_Matrix(5, 0);

        p.xp = mParticles[i].x;
        p.yp = mParticles[i].y;
        p.zp = mParticles[i].z;

        p.x0 = mParticles[i].x0;
        p.y0 = mParticles[i].y0;
        p.z0 = mParticles[i].z0;
        p.w = 0;

        mParticles[i].x = p.x;
        mParticles[i].y = p.y;
        mParticles[i].z = p.z;

        mParticles[i].xp = p.xp;
        mParticles[i].yp = p.yp;
        mParticles[i].zp = p.zp;

        mParticles[i].x0 = p.x0;
        mParticles[i].y0 = p.y0;
        mParticles[i].z0 = p.z0;

        mParticles[i].x_velocity = p.x_velocity;
        mParticles[i].y_velocity = p.y_velocity;
        mParticles[i].z_velocity = p.z_velocity;
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
    mMaxLikelihood = 0.0f;

    for(int i = 0; i < mParams.NUMBER_OF_PARTICLES; ++i) {
        float particleWeight = mParticles[i].w;
        sum += particleWeight;

        if (particleWeight > mMaxLikelihood)
            mMaxLikelihood = particleWeight;
    }

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
    pose(0) = 0.0;//mParticles[0].x;
    pose(1) = 0.0;//mParticles[0].y;
    pose(2) = 0.0;//mParticles[0].z;


    for(int i=0; i<mParams.NUMBER_OF_PARTICLES; ++i)
    {
        pose(0)+=mParticles[i].x;
        pose(1)+=mParticles[i].y;
        pose(2)+=mParticles[i].z;
    }

    if(mParams.NUMBER_OF_PARTICLES>0)
    {
        pose(0) /=mParams.NUMBER_OF_PARTICLES;
        pose(1) /=mParams.NUMBER_OF_PARTICLES;
        pose(2) /=mParams.NUMBER_OF_PARTICLES;
    }
    else
    {
        pose(0) = 0.0;//mParticles[0].x;
        pose(1) = 0.0;//mParticles[0].y;
        pose(2) = 0.0;//mParticles[0].z;
    }

    //std::cout << "Output pose " << pose[0] << " " << pose[1] << " " << pose[2] << std::endl;
}

void ParticleFilter::getParticlePose(int id, cv::Vec3d& pose)
{

    pose(0) = mParticles[id].x;
    pose(1) = mParticles[id].y;
    pose(2) = mParticles[id].z;

}

void ParticleFilter::getOutputVelocity(cv::Vec3d& vel)
{
    vel(0) = mParticles[0].x;
    vel(1) = mParticles[0].y;
    vel(2) = mParticles[0].z;
}

uint ParticleFilter::getFilterId()
{
    return filterId;
}


float ParticleFilter::getLikelihood() {
  return mMaxLikelihood;
}

}
