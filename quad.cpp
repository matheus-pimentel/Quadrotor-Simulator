#include "quad.h"
#include "windows.h"
#include "iostream"
#include "math.h"
#include "utils.h"

using namespace std;

quad::quad()
{
    init_quad();
}

void quad::run()
{
    while(is_running){

        motor = controlhandle->update_motors(t,state);
        model();
        cout << "state " << endl;
        print_matrix(state);
        Sleep(100);
    }
}

void quad::init_quad()
{
    position = resize_matrix(1,3);
    orientation = resize_matrix(1,3);
    linear_vel = resize_matrix(1,3);
    linear_acc = resize_matrix(1,3);
    angular_vel = resize_matrix(1,3);
    angular_vel_quad = resize_matrix(1,3);
    angular_acc = resize_matrix(1,3);
    state = resize_matrix(4,3);
    old_state = resize_matrix(4,3);
    des_state = resize_matrix(5,3);
    old_des_state = resize_matrix(5,3);
    motor = resize_matrix(1,4);
    b3 = resize_matrix(1,3);
    b3.matrix = {{0,0,1}};

    set_params(1,0.468);
    set_params(2,0.225);
    set_params(3,1.14*pow(10,-7));
    set_params(4,2.98*pow(10,-6));
    set_params(5,0.004856);
    set_params(6,0.004856);
    set_params(7,0.008801);
    set_params(8,9.81);
    set_params(9,0.02);

    controlhandle = new controller;
    controlhandle->set_params(quad_params.mass,quad_params.dt,quad_params.gravity,quad_params.Ixx,quad_params.Iyy,quad_params.Izz,quad_params.b,quad_params.k,quad_params.l);

    waypoints = resize_matrix(100,5);
    waypoints.l = 2;
    waypoints.matrix = {{0,0,0,0,0},
                        {1,1,1,1,1}};
    controlhandle->set_waypoints(waypoints);

}

void quad::model()
{
    double roll = orientation.matrix[0][0];
    double pitch = orientation.matrix[0][1];
    double yaw = orientation.matrix[0][2];

    matrixds vectora, vectorb;
    vectora = resize_matrix(1,3);
    vectorb = resize_matrix(1,3);

    R = rotation_matrix(roll,pitch,yaw);

    double thrust = quad_params.k*(product_matrix(motor,transposed_matrix(motor)).matrix[0][0])/quad_params.mass;
    thrust = motor.matrix[0][0]/quad_params.mass;

    linear_acc = sum_matrix(multiple_matrix((-quad_params.gravity),b3),transposed_matrix(multiple_matrix(thrust,column_matrix(R,2))));
    linear_vel = sum_matrix(linear_vel,multiple_matrix(quad_params.dt,linear_acc));
    position = sum_matrix(position,multiple_matrix(quad_params.dt,linear_vel));

    /*****************************************************************************************************
    double p = angular_vel_quad.matrix[0][0];
    double q = angular_vel_quad.matrix[0][1];
    double r = angular_vel_quad.matrix[0][2];

    vectora.matrix = {{((quad_params.Iyy-quad_params.Izz)*q*r/quad_params.Ixx),
                       ((quad_params.Izz-quad_params.Ixx)*p*r/quad_params.Iyy),
                       ((quad_params.Ixx-quad_params.Iyy)*p*q/quad_params.Izz)}};
    vectorb.matrix = {{(quad_params.l*quad_params.k*(pow(motor.matrix[0][1],2)-pow(motor.matrix[0][3],2))/quad_params.Ixx),
                       (quad_params.l*quad_params.k*(-pow(motor.matrix[0][0],2)+pow(motor.matrix[0][2],2))/quad_params.Iyy),
                       (quad_params.b*(pow(motor.matrix[0][0],2)-pow(motor.matrix[0][1],2)+pow(motor.matrix[0][2],2)-pow(motor.matrix[0][3],2))/quad_params.Izz)}};

    angular_acc = sum_matrix(vectora,vectorb);
    *****************************************************************************************************/

    matrixds I = resize_matrix(3,3), vec = resize_matrix(3,1);
    I.matrix = {{quad_params.Ixx,0,0},
         {0,quad_params.Iyy,0},
         {0,0,quad_params.Izz}};
    vec.matrix = {{motor.matrix[0][1]},{motor.matrix[0][2]},{motor.matrix[0][3]}};
    angular_acc = transposed_matrix(product_matrix(inverse_matrix(I),vec));

    angular_vel_quad = sum_matrix(angular_vel_quad,multiple_matrix(quad_params.dt,angular_acc));
    angular_vel = transposed_matrix(product_matrix(inv_transformation_matrix(roll,pitch,yaw),transposed_matrix(angular_vel_quad)));
    orientation = sum_matrix(orientation,multiple_matrix(quad_params.dt,angular_vel));

    if(orientation.matrix[0][0] > PI){
        orientation.matrix[0][0] = orientation.matrix[0][0] - 2*PI;
    }
    if(orientation.matrix[0][0] < -PI){
        orientation.matrix[0][0] = orientation.matrix[0][0] + 2*PI;
    }
    if(orientation.matrix[0][1] > PI){
        orientation.matrix[0][1] = orientation.matrix[0][1] - 2*PI;
    }
    if(orientation.matrix[0][1] < -PI){
        orientation.matrix[0][1] = orientation.matrix[0][1] + 2*PI;
    }

    old_state = state;
    state.matrix[0] = {{position.matrix[0][0], position.matrix[0][1], position.matrix[0][2]}};
    state.matrix[1] = {{linear_vel.matrix[0][0], linear_vel.matrix[0][1], linear_vel.matrix[0][2]}};
    state.matrix[2] = {{orientation.matrix[0][0], orientation.matrix[0][1], orientation.matrix[0][2]}};
    state.matrix[3] = {{angular_vel_quad.matrix[0][0], angular_vel_quad.matrix[0][1], angular_vel_quad.matrix[0][2]}};

    t = t + quad_params.dt;
    iteration++;
}

void quad::set_params(int select, double value)
{
    switch (select) {
    case 1:
        quad_params.mass = value;
        break;
    case 2:
        quad_params.l = value;
        break;
    case 3:
        quad_params.b = value;
        break;
    case 4:
        quad_params.k = value;
        break;
    case 5:
        quad_params.Ixx = value;
        break;
    case 6:
        quad_params.Iyy = value;
        break;
    case 7:
        quad_params.Izz = value;
        break;
    case 8:
        quad_params.gravity = value;
        break;
    case 9:
        quad_params.dt = value;
        break;
    default:
        break;
    }
}

void quad::set_waypoints(matrixds matrix)
{
    waypoints.matrix[waypoints.l] = {{matrix.matrix[0][0], matrix.matrix[0][1], matrix.matrix[0][2], matrix.matrix[0][3], matrix.matrix[0][4]}};
    waypoints.l++;
    controlhandle->set_waypoints(waypoints);
}

matrixds quad::get_waypoints()
{
    return waypoints;
}

void quad::set_run(bool a)
{
    is_running = a;
}

quad::~quad()
{

}
