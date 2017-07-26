#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 30;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 30;

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */

  // State dimension
  n_x_ = 5;

  // Augmented state dimension
  n_aug_ = 7;

  // Sigma point spreading parameter
  lambda_ = 3-n_x_;

  is_initialized_ = false;

  // initial state covariance matrix
  P_<< 1,0,0,0,0,
       0,1,0,0,0,
       0,0,1,0,0,
       0,0,0,1,0,
       0,0,0,0,1;
  //initial state value
  x_ << 0,0,0,0,0;

  // time when the state is true, in us
  time_us_ = 0; // work as previous_timestamp

  // predicted sigma points matrix
  Xsig_pred_ = MatrixXd(n_x_, 2 * n_aug_ + 1);
    
    //create vector for weights
    weights_ = VectorXd(2*n_aug_+1);
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
    // Initialization and use first measurement to set up px and py
    if (!is_initialized_) {



      if (meas_package.sensor_type_ == MeasurementPackage::RADAR) {
        /**
        Convert radar from polar to cartesian coordinates and initialize state.
        */
        float ro = meas_package.raw_measurements_[0];
        float theta = meas_package.raw_measurements_[1];
        float ro_dot = meas_package.raw_measurements_[2];

        x_ << ro*cos(theta),ro*sin(theta),0,0,0;
        time_us_ = meas_package.timestamp_;
      }
      else if (meas_package.sensor_type_ == MeasurementPackage::LASER) {
        /**
        Initialize state.
        */
        x_ << meas_package.raw_measurements_[0],meas_package.raw_measurements_[1],0,0,0;
        time_us_ = meas_package.timestamp_;

      }





      // done initializing, no need to predict or update
      is_initialized_ = true;
      return;
    }


    //compute the time elapsed between the current and previous measurements
    float dt = (meas_package.timestamp_ - time_us_) / 1000000.0;	//dt - expressed in seconds
    time_us_ = meas_package.timestamp_;

    Prediction(dt);



}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
    /* It seems no need to get normal sigma points only use augmentation
    //create sigma point matrix
    MatrixXd Xsig = MatrixXd(n_x_, 2 * n_x_ + 1);

    //calculate square root of P
    MatrixXd A = P_.llt().matrixL();

    //set first column of sigma point matrix
     Xsig.col(0)  = x_;

     //set remaining sigma points
     for (int i = 0; i < n_x_; i++)
     {
       Xsig.col(i+1)     = x + sqrt(lambda_+n_x_) * A.col(i);
       Xsig.col(i+1+n_x) = x - sqrt(lambda_+n_x_) * A.col(i);
     }*/

    //** Start generate augmented sigma points
    //create augmented mean vector
    VectorXd x_aug = VectorXd(n_aug_);

    //create augmented state covariance
    MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);

    //create sigma point matrix
    MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

    //create augmented mean state
    x_aug.head(5) = x_;
    x_aug(5) = 0;
    x_aug(6) = 0;

    //create augmented covariance matrix
    P_aug.fill(0.0);
    P_aug.topLeftCorner(5,5) = P_;
    P_aug(5,5) = std_a_*std_a_;
    P_aug(6,6) = std_yawdd_*std_yawdd_;

    //create square root matrix
    MatrixXd L = P_aug.llt().matrixL();

    //create augmented sigma points
    Xsig_aug.col(0)  = x_aug;
    for (int i = 0; i< n_aug_; i++)
    {
      Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
      Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
    }

    //** Start to predict sigma points
    //predict sigma points
      //avoid division by zero
      //write predicted sigma points into right column
      for(int i=0;i<2*n_aug_+1;i++){
          double px = Xsig_aug(0,i);
          double py = Xsig_aug(1,i);
          double v = Xsig_aug(2,i);
          double yaw = Xsig_aug(3,i);
          double yawd = Xsig_aug(4,i);
          double nu_a = Xsig_aug(5,i);
          double nu_yawdd = Xsig_aug(6,i);

          //predicted x state
          double px_p;
          double py_p;

          // check yawd is zero or not
          if(fabs(yawd)<0.01){ //yawd is zero
              px_p = px + (v*cos(yaw)*delta_t);
              py_p = py + (v*sin(yaw)*delta_t);
          }else{ // yawd is not zero
              px_p = px + v*(sin(yaw+yawd*delta_t)-sin(yaw))/yawd;
              py_p = py + v*(-cos(yaw+yawd*delta_t)+cos(yaw))/yawd;
          }

          //adding noise effect to predicted state
          px_p += 0.5*delta_t*delta_t*cos(yaw)*nu_a;
          py_p += 0.5*delta_t*delta_t*sin(yaw)*nu_a;

          // v state predict
          double v_p ;
          v_p = v +delta_t*nu_a;

          // predict yaw and yawd
          double yaw_p,yawd_p;
          yaw_p = yaw + yawd*delta_t + (0.5*delta_t*delta_t*nu_yawdd);
          yawd_p = yawd + delta_t*nu_yawdd;

          // put back
          Xsig_pred_(0,i)=px_p;
          Xsig_pred_(1,i)=py_p;
          Xsig_pred_(2,i)=v_p;
          Xsig_pred_(3,i)=yaw_p;
          Xsig_pred_(4,i)=yawd_p;

      }


    //*** Start to Predict Mean and Convariance
    //set weights
    weights_(0) = lambda_/(lambda_+n_aug_);
    for(int i=1;i<2*n_aug_+1;i++){
        double weight = 0.5/(lambda_+n_aug_);
        weights_(i)= weight;
    }
    //std::cout << "pass" << std::endl;
    //predict state mean
    x_.fill(0.0);
    for(int i=0;i<2*n_aug_+1;i++){
        x_ += weights_(i)*Xsig_pred_.col(i);
    }
    
    //std::cout << x_ << std::endl;
    
    //predict state covariance matrix
    P_.fill(0.0);
    for(int i=0;i<2*n_aug_+1;i++){
        VectorXd diff = Xsig_pred_.col(i)-x_;
        //angle normalization
        while (diff(3)> M_PI) diff(3)-=2.*M_PI;
        while (diff(3)<-M_PI) diff(3)+=2.*M_PI;
        
        P_ += weights_(i)*diff*diff.transpose();
        
    }


}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
}
