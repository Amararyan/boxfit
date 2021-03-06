////////////////////////////////////////////////////////////////////////////////
// eds_2D.h
//
// Created November 17, 2010 by HJvE
// Last Modified January 16, 2012 by HJvE
//
////////////////////////////////////////////////////////////////////////////////

#include "eds_2D_regular.h"

////////////////////////////////////////////////////////////////////////////////
// Static member definitions

s_ray** c_eds :: ray;

////////////////////////////////////////////////////////////////////////////////
// constructor en destructor
////////////////////////////////////////////////////////////////////////////////

c_eds :: c_eds()
{
  // set up initialization values for parameters
  ur_rays = 768;
  uphi_rays = 32;
  memory_assigned = false;
  #if BOOST_ == ENABLED_
    boost = 1.0, boostsqr = 1.0, beta_sim = 0.0; // start with nonmoving grid
  #endif // BOOST_
}

////////////////////////////////////////////////////////////////////////////////

void c_eds :: initialize()
{
  // assign memory
    
  if (memory_assigned) delete_array_2d(ray);
  ray = array_2d<s_ray>(ur_rays + 1, uphi_rays + 1);
  memory_assigned = true;
}

////////////////////////////////////////////////////////////////////////////////

c_eds :: ~c_eds()
{
  if (memory_assigned) delete_array_2d(ray);
}

////////////////////////////////////////////////////////////////////////////////
// public functions
////////////////////////////////////////////////////////////////////////////////

// Prepare update routine for global approach to synchrotron self-absorption

#if LOCAL_SELF_ABSORPTION_ == DISABLED_

  void c_eds :: prepare_update()
  {
    // set local emission and absorption coefficients for all rays
  
    double r2;
    s_coordinates cor;
    #if BOOST_ == ENABLED_
      double Ex, Ey, Ez; // projections on EDS through origin
    #endif // BOOST_
    int iur, iuphi;

    // loop over all rays
    for (iur = 0; iur < ur_rays; iur++)
    {
      for (iuphi = 0; iuphi < uphi_rays; iuphi++)
      {
        #if BOOST_ == DISABLED_

          // get coordinates in grid frame
          cor.x = O[0] + ray[iur][iuphi].ux * eux[0] + 
            ray[iur][iuphi].uy * euy[0];
          cor.y = O[1] + ray[iur][iuphi].ux * eux[1] + 
            ray[iur][iuphi].uy * euy[1];
          cor.z = O[2] + ray[iur][iuphi].ux * eux[2] + 
            ray[iur][iuphi].uy * euy[2];
          cor.t = t_sim;
    
        #else // BOOST_

          // if boost, the EDS is curved
          Ex = ray[iur][iuphi].ux * eux[0] + ray[iur][iuphi].uy * euy[0];
          Ey = ray[iur][iuphi].ux * eux[1] + ray[iur][iuphi].uy * euy[1];
          Ez = ray[iur][iuphi].ux * eux[2] + ray[iur][iuphi].uy * euy[2];

          cor.z = (Ez + p_Obs->costheta * v_light * 
            (t_sim / boost - p_Obs->t / (p_Obs->z + 1.))) / 
            (1.0 - beta_sim * p_Obs->costheta);

          cor.t = t_sim / boost + beta_sim * cor.z * invv_light;

          cor.x = Ex + (cor.t - p_Obs->t / (p_Obs->z + 1.)) * 
            p_Obs->sintheta * v_light;

          cor.y = Ey;

        #endif // BOOST_
  
        // get emmission and absorption coefficients
        r2 = cor.x * cor.x + cor.y * cor.y + cor.z * cor.z;
        if (r2 >= v_light * v_light * cor.t * cor.t)
        {
          ray[iur][iuphi].em = 0.; ray[iur][iuphi].ab = 0.;
        }
        else
        {
          // get emission and absorption coefficients
          p_emab->get_emab(cor, ray[iur][iuphi].em, ray[iur][iuphi].ab);
        }
      }
    }
  }

#endif // LOCAL_SELF_ABSORPTION == DISABLED_

////////////////////////////////////////////////////////////////////////////////
// update routine for local approach to synchrotron self-absorption

#if LOCAL_SELF_ABSORPTION_ == ENABLED_

  void c_eds :: prepare_update(int RK, double dt_sim)
  {
    int iur, iuphi;
    double tau, S, expo;
    s_coordinates cor;
    #if BOOST_ == ENABLED_
      double Ex, Ey, Ez; // projections on EDS through origin
    #endif // BOOST_

    double r2;

    #if BOOST_ == ENABLED_
      Dr = dt_sim * v_light / (boost * (1.0 - beta_sim * p_Obs->costheta));
    #endif // BOOST_
    #if BOOST_ == DISABLED_
      Dr = dt_sim * v_light;
    #endif // BOOST_

    //--------------------------------------------------------------------------
    // loop over all rays to determine em and ab

    if (RK != 1 and RK != 3)
    { 
      for (iur = 0; iur <= ur_rays; iur++)
      {
        for (iuphi = 0; iuphi <= uphi_rays; iuphi++)
        {
          #if BOOST_ == DISABLED_

            // get coordinates in grid frame
            cor.x = O[0] + ray[iur][iuphi].ux * eux[0] + 
              ray[iur][iuphi].uy * euy[0];
            cor.y = O[1] + ray[iur][iuphi].ux * eux[1] + 
              ray[iur][iuphi].uy * euy[1];
            cor.z = O[2] + ray[iur][iuphi].ux * eux[2] + 
              ray[iur][iuphi].uy * euy[2];
            cor.t = t_sim;
        
          #else
  
            // if boost, the EDS is curved for off-axis observers
            Ex = ray[iur][iuphi].ux * eux[0] + ray[iur][iuphi].uy * euy[0];
            Ey = ray[iur][iuphi].ux * eux[1] + ray[iur][iuphi].uy * euy[1];
            Ez = ray[iur][iuphi].ux * eux[2] + ray[iur][iuphi].uy * euy[2];

            cor.z = (Ez + p_Obs->costheta * v_light * 
              (t_sim / boost - p_Obs->t / (p_Obs->z + 1.))) / 
              (1.0 - beta_sim * p_Obs->costheta);

            cor.t = t_sim / boost + beta_sim * cor.z * invv_light;

            cor.x = Ex + (cor.t - p_Obs->t / (p_Obs->z + 1.)) * 
              p_Obs->sintheta * v_light;

            cor.y = Ey;

          #endif // BOOST_

          r2 = cor.x * cor.x + cor.y * cor.y + cor.z * cor.z;
          if (r2 >= v_light * v_light * cor.t * cor.t)
          {
            ray[iur][iuphi].em = 0.; ray[iur][iuphi].ab = 0.;
          }
          else
          {
            // get emission and absorption coefficients
            p_emab->get_emab(cor, ray[iur][iuphi].em, ray[iur][iuphi].ab);
          }
        }  // loop over iuphi
      }  // loop over iur
    } // RK != 1 and RK != 3

    //--------------------------------------------------------------------------
    // apply em and ab
  
    for (iur = 0; iur <= ur_rays; iur++)
    {
      for (iuphi = 0; iuphi <= uphi_rays; iuphi++)
      {
        tau = ray[iur][iuphi].ab * Dr;
 
        // store in the approprate Runge-Kutta bin
        switch(RK)
        {
          case 1:
            if (tau > 1e-3)
            {
              expo = exp(-tau);
              S = ray[iur][iuphi].em / (ray[iur][iuphi].ab + 1e-200);
              ray[iur][iuphi].k1 = ray[iur][iuphi].I * (expo - 1.) + 
              S * (1.0 - expo);
            }
            else
            {
              ray[iur][iuphi].k1 = ray[iur][iuphi].I * tau * (.5 * tau - 1.) +
                ray[iur][iuphi].em * Dr * (1. - 0.5 * tau);
            }
          break;
          case 2:
            if (tau > 1e-3)
            {
              expo = exp(-tau);
              S = ray[iur][iuphi].em / (ray[iur][iuphi].ab + 1e-200);
              ray[iur][iuphi].k2 = (ray[iur][iuphi].I + 
                0.5 * ray[iur][iuphi].k1) * (expo - 1.) + S * (1. - expo);
            }
            else
            {
              ray[iur][iuphi].k2 = (ray[iur][iuphi].I + 
                0.5 * ray[iur][iuphi].k1) * tau * (0.5 * tau - 1.) +
                ray[iur][iuphi].em * Dr * (1. - 0.5 * tau);
            }
          break;
          case 3:
            if (tau > 1e-3)
            {
              expo = exp(-tau);
              S = ray[iur][iuphi].em / (ray[iur][iuphi].ab + 1e-200);
              ray[iur][iuphi].k3 = (ray[iur][iuphi].I + 
                0.5 * ray[iur][iuphi].k2) * (expo - 1.) + S * (1. - expo);
            }
            else
            {
              ray[iur][iuphi].k3 = (ray[iur][iuphi].I + 
                0.5 * ray[iur][iuphi].k2) * tau * (0.5 * tau - 1.) +
                ray[iur][iuphi].em * Dr * (1. - 0.5 * tau);
            }
          break;
          case 4:
            if (tau > 1e-3)
            {
              expo = exp(-tau);
              S = ray[iur][iuphi].em / (ray[iur][iuphi].ab + 1e-200);
              ray[iur][iuphi].k4 = (ray[iur][iuphi].I + 
                ray[iur][iuphi].k3) * (expo - 1.) + S * (1. - expo);
            }
            else
            {
              ray[iur][iuphi].k4 = (ray[iur][iuphi].I + 
                ray[iur][iuphi].k3) * tau * (0.5 * tau - 1.) + 
                ray[iur][iuphi].em * Dr * (1. - 0.5 * tau);
            }
          break;
        } // switch
      } // iuphi loop
    } // iur loop
  }

#endif // LOCAL_SELF_ABSORPTION_ == ENABLED_

////////////////////////////////////////////////////////////////////////////////

#if LOCAL_SELF_ABSORPTION_ == DISABLED_

  void c_eds :: finalize_update(double dt_sim) 
  { 
    int iur, iuphi;

    #if BOOST_ == ENABLED_
      Dr = dt_sim * v_light / (boost * (1.0 - beta_sim * p_Obs->costheta));
    #endif // BOOST_
    #if BOOST_ == DISABLED_
    Dr = dt_sim * v_light;
    #endif // BOOST_

    // loop over all rays
    for (iur = 0; iur < ur_rays; iur++)
    {
      for (iuphi = 0; iuphi < uphi_rays; iuphi++)
      {
        ray[iur][iuphi].emdr += ray[iur][iuphi].em * Dr;
        ray[iur][iuphi].abdr += ray[iur][iuphi].ab * Dr;
      }
    }
  }

#endif // LOCAL_SELF_ABSORPTION_ == DISABLED_

////////////////////////////////////////////////////////////////////////////////

#if LOCAL_SELF_ABSORPTION_ == DISABLED_

  void c_eds :: finalize_lores_update(double dt_sim)  
  {
    int iur, iuphi;

    #if BOOST_ == ENABLED_
      Dr = dt_sim * v_light / (boost * (1.0 - beta_sim * p_Obs->costheta));
    #endif // BOOST_
    #if BOOST_ == DISABLED_
      Dr = dt_sim * v_light;
    #endif // BOOST_

    // loop over all rays
    for (iur = 0; iur < ur_rays; iur++)
    {
      for (iuphi = 0; iuphi < uphi_rays; iuphi++)
      {
        ray[iur][iuphi].emdr_lores += ray[iur][iuphi].em * Dr;
        ray[iur][iuphi].abdr_lores += ray[iur][iuphi].ab * Dr;
      }
    }
  }

#endif // LOCAL_SELF_ABSORPTION_ == DISABLED_

////////////////////////////////////////////////////////////////////////////////

#if LOCAL_SELF_ABSORPTION_ == ENABLED_

  void c_eds :: finalize_RK_update()
  {
    int iur, iuphi;
    double Iold;

    for (iur = 0; iur < ur_rays; iur++)
    {
      for (iuphi = 0; iuphi < uphi_rays; iuphi++)
      {
        Iold = ray[iur][iuphi].I;
        ray[iur][iuphi].I = fmax((Iold + 
          ray[iur][iuphi].k1 / 6. +
          ray[iur][iuphi].k2 / 3. +
          ray[iur][iuphi].k3 / 3. +
          ray[iur][iuphi].k4 / 6.), 0.);

        // traced low resolution as well, for determining error due to time step
        Iold = ray[iur][iuphi].I_lores;
        ray[iur][iuphi].I_lores = fmax(Iold + ray[iur][iuphi].k1, 0.);
      } // iuphi
    } // iur
  }

#endif // LOCAL_SELF_ABSORPTION_ == ENABLED_

////////////////////////////////////////////////////////////////////////////////

void c_eds :: set_coordinates(double a_t_sim)
{
  // The coordinates of the EDS are as follows. In 1D the normal to the eds
  // lies along the z-axis of the grid. The x-direction on the EDS lies along
  // the grid x-axis, the y on the EDS along the grid y-axis. If the EDS is
  // tilted (i.e. the observer angle is nonzero), the normal to the eds surface
  // moves in the x-z plane. As a consequence the y direction on the EDS remains
  // unaffected. The x direction on the EDS no longer coincides with the x
  // direction on the grid.

  t_sim = a_t_sim;

  #if BOOST_ == DISABLED_
    double r = (t_sim - p_Obs->t / (p_Obs->z + 1.)) * v_light;
    O[0] = r * p_Obs->sintheta; O[1] = 0.0; O[2] = r * p_Obs->costheta;
    O_r = r; // O_r < 0 if grid origin between eds and observer
  #endif
  
  // set unit vectors on the EDS plane in the frame of the grid
  eux[0] = p_Obs->costheta; eux[1] = 0.0; eux[2] = -p_Obs->sintheta;
  euy[0] = 0.0; euy[1] = 1.0; euy[2] = 0.0;
}

////////////////////////////////////////////////////////////////////////////////
// internal ( protected ) functions c_eds_2D
////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_F_annulus(int iur)
{
  #if LOCAL_SELF_ABSORPTION_ == ENABLED_

    // on-axis observer
    if (uphi_rays == 1) 
      return ray[iur][0].I * PI * ray[iur][0].ur * ray[iur][0].ur;

    // off-axis observer
    double h = PI / uphi_rays;
    double F = 0.;
    int iuphi;

    for (iuphi = 0; iuphi < uphi_rays / 5; iuphi++)
      F += 5. * h / 288. * (
        19. * ray[iur][iuphi * 5].I +
        75. * ray[iur][iuphi * 5 + 1].I +
        50. * ray[iur][iuphi * 5 + 2].I +
        50. * ray[iur][iuphi * 5 + 3].I +
        75. * ray[iur][iuphi * 5 + 4].I +
        19. * ray[iur][iuphi * 5 + 5].I);

    return F * ray[iur][0].ur * ray[iur][0].ur;
  
  #else // LOCAL_SELF_ABSORPTION_ == DISABLED_

    // on-axis observer
    if (uphi_rays == 1) 
    {
      double S, tau;
  
      if (ray[iur][0].abdr > 1e-3)
      {
        S = ray[iur][0].emdr / ray[iur][0].abdr;
        tau = ray[iur][0].abdr;
        return S * (1. - exp(-tau)) * PI * ray[iur][0].ur * ray[iur][0].ur;
      }
      else
        return ray[iur][0].emdr * (1. - 0.5 * ray[iur][0].abdr) * PI * 
          ray[iur][0].ur * ray[iur][0].ur;
    }

    // off-axis observer
    double F = 0.;
    double h = PI / uphi_rays;
    int iuphi;
    double S[6], tau[6], dF[6];
    int i;

    for (iuphi = 0; iuphi < uphi_rays / 5; iuphi++)
    {
      for (i = 0; i < 6; i++)
      {
        tau[i] = ray[iur][iuphi * 5 + i].abdr;
        if (tau[i] > 1e-3)
        {
          S[i] = ray[iur][iuphi * 5 + i].emdr / ray[iur][iuphi * 5 + i].abdr;
          dF[i] = S[i] * (1. - exp(-tau[i]));
        }
        else
        {
          dF[i] = ray[iur][iuphi * 5 + i].emdr * (1. - 0.5 * tau[i]);
        }
      }

      F += 5. * h / 288. * (19. * dF[0] + 75. * dF[1] + 50. * dF[2] + 
        50. * dF[3] + 75. * dF[4] + 19. * dF[5] );
    }

    return F * ray[iur][0].ur * ray[iur][0].ur;

  #endif
}

////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_F_annulus_lores_t(int iur)
{
  #if LOCAL_SELF_ABSORPTION_ == ENABLED_

    // on-axis observer
    if (uphi_rays == 1) 
      return ray[iur][0].I_lores * PI * ray[iur][0].ur * ray[iur][0].ur;

    // off-axis observer
    double h = PI / uphi_rays;
    double F = 0.;
    int iuphi;

    for (iuphi = 0; iuphi < uphi_rays / 5; iuphi++)
      F += 5. * h / 288. * (
        19. * ray[iur][iuphi * 5].I_lores +
        75. * ray[iur][iuphi * 5 + 1].I_lores +
        50. * ray[iur][iuphi * 5 + 2].I_lores +
        50. * ray[iur][iuphi * 5 + 3].I_lores +
        75. * ray[iur][iuphi * 5 + 4].I_lores +
        19. * ray[iur][iuphi * 5 + 5].I_lores);

    return F * ray[iur][0].ur * ray[iur][0].ur;
  
  #else // LOCAL_SELF_ABSORPTION_ == DISABLED_

    // on-axis observer
    if (uphi_rays == 1) 
    {
      double S, tau;
  
      if (ray[iur][0].abdr_lores > 1e-3)
      {
        S = ray[iur][0].emdr_lores / ray[iur][0].abdr_lores;
        tau = ray[iur][0].abdr_lores;
        return S * (1. - exp(-tau)) * PI * ray[iur][0].ur * ray[iur][0].ur;
      }
      else
        return ray[iur][0].emdr_lores * 
          (1. - 0.5 * ray[iur][0].abdr_lores) * PI * 
          ray[iur][0].ur * ray[iur][0].ur;
    }

    // off-axis observer
    double F = 0.;
    double h = PI / uphi_rays;
    int iuphi;
    double S[6], tau[6], dF[6];
    int i;

    for (iuphi = 0; iuphi < uphi_rays / 5; iuphi++)
    {
      for (i = 0; i < 6; i++)
      {
        tau[i] = ray[iur][iuphi * 5 + i].abdr_lores;
        if (tau[i] > 1e-3)
        {
          S[i] = ray[iur][iuphi * 5 + i].emdr_lores / 
            ray[iur][iuphi * 5 + i].abdr_lores;
          dF[i] = S[i] * (1. - exp(-tau[i]));
        }
        else
        {
          dF[i] = ray[iur][iuphi * 5 + i].emdr_lores * (1. - 0.5 * tau[i]);
        }
      }

      F += 5. * h / 288. * (19. * dF[0] + 75. * dF[1] + 50. * dF[2] + 
        50. * dF[3] + 75. * dF[4] + 19. * dF[5] );
    }

    return F * ray[iur][0].ur * ray[iur][0].ur;

  #endif
}

////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_F_annulus_lores_phi(int iur)
{
  #if LOCAL_SELF_ABSORPTION_ == ENABLED_

    // on-axis observer
    if (uphi_rays == 1) 
      return ray[iur][0].I * PI * ray[iur][0].ur * ray[iur][0].ur;

    // off-axis observer
    double h = 2 * PI / uphi_rays; // note the increased step size
    double F = 0.;
    int iuphi;

    for (iuphi = 0; iuphi < uphi_rays / 5; iuphi += 2)
      F += 5. * h / 288. * (
        19. * ray[iur][iuphi * 5].I +
        75. * ray[iur][iuphi * 5 + 1].I +
        50. * ray[iur][iuphi * 5 + 2].I +
        50. * ray[iur][iuphi * 5 + 3].I +
        75. * ray[iur][iuphi * 5 + 4].I +
        19. * ray[iur][iuphi * 5 + 5].I);

    return F * ray[iur][0].ur * ray[iur][0].ur;
  
  #else // LOCAL_SELF_ABSORPTION_ == DISABLED_

    // on-axis observer
    if (uphi_rays == 1) 
    {
      double S, tau;
  
      if (ray[iur][0].abdr > 1e-3)
      {
        S = ray[iur][0].emdr / ray[iur][0].abdr;
        tau = ray[iur][0].abdr;
        return S * (1. - exp(-tau)) * PI * ray[iur][0].ur * ray[iur][0].ur;
      }
      else
        return ray[iur][0].emdr * (1. - 0.5 * ray[iur][0].abdr) * PI * 
          ray[iur][0].ur * ray[iur][0].ur;
    }

    // off-axis observer
    double F = 0.;
    double h = 2 * PI / uphi_rays;
    int iuphi;
    double S[6], tau[6], dF[6];
    int i;

    for (iuphi = 0; iuphi < uphi_rays / 5; iuphi += 2)
    {
      for (i = 0; i < 6; i++)
      {
        tau[i] = ray[iur][iuphi * 5 + i].abdr;
        if (tau[i] > 1e-3)
        {
          S[i] = ray[iur][iuphi * 5 + i].emdr / ray[iur][iuphi * 5 + i].abdr;
          dF[i] = S[i] * (1. - exp(-tau[i]));
        }
        else
        {
          dF[i] = ray[iur][iuphi * 5 + i].emdr * (1. - 0.5 * tau[i]);
        }
      }

      F += 5. * h / 288. * (19. * dF[0] + 75. * dF[1] + 50. * dF[2] + 
        50. * dF[3] + 75. * dF[4] + 19. * dF[5] );
    }

    return F * ray[iur][0].ur * ray[iur][0].ur;

  #endif
}

////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_total_flux()
{
  int iur; // , iuphi;
  double F = 0.;
  double h = (log(ur_max) - log(ur_min)) / (double) ur_rays;

  for (iur = 0; iur < ur_rays / 5; iur++)
    F += 5 * h / 288. * (
      19. * get_F_annulus(iur * 5) +
      75. * get_F_annulus(iur * 5 + 1) +
      50. * get_F_annulus(iur * 5 + 2) +
      50. * get_F_annulus(iur * 5 + 3) +
      75. * get_F_annulus(iur * 5 + 4) +
      19. * get_F_annulus(iur * 5 + 5));

  F = (2.0 * F) * (1.0 + p_Obs->z) / (p_Obs->dL * p_Obs->dL);
  return F;
}

////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_F_r_error()
{
  // returns measure of error due to radial resolution
  double F = get_total_flux();
  double Flores = 0.;
  int iur;
  double h = (log(ur_max) - log(ur_min)) * 2. / (double) ur_rays;

  for (iur = 0; iur < ur_rays / 5; iur += 2)
    Flores += 5 * h / 288. * (
      19. * get_F_annulus(iur * 5) +
      75. * get_F_annulus(iur * 5 + 1) +
      50. * get_F_annulus(iur * 5 + 2) +
      50. * get_F_annulus(iur * 5 + 3) +
      75. * get_F_annulus(iur * 5 + 4) +
      19. * get_F_annulus(iur * 5 + 5));

  Flores = (2.0 * Flores) * (1.0 + p_Obs->z) / (p_Obs->dL * p_Obs->dL);
  
  return 2. * fabs((Flores - F) / (Flores + F + 1e-200));
}

////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_F_phi_error()
{
  // returns measure of error due to angular resolution
  double F = get_total_flux();

  int iur;
  double Flores = 0.;
  double h = (log(ur_max) - log(ur_min)) / (double) ur_rays;

  for (iur = 0; iur < ur_rays / 5; iur++)
    Flores += 5 * h / 288. * (
      19. * get_F_annulus_lores_phi(iur * 5) +
      75. * get_F_annulus_lores_phi(iur * 5 + 1) +
      50. * get_F_annulus_lores_phi(iur * 5 + 2) +
      50. * get_F_annulus_lores_phi(iur * 5 + 3) +
      75. * get_F_annulus_lores_phi(iur * 5 + 4) +
      19. * get_F_annulus_lores_phi(iur * 5 + 5));

  Flores = (2.0 * Flores) * (1.0 + p_Obs->z) / (p_Obs->dL * p_Obs->dL);
  
  return 2. * fabs((Flores - F) / (Flores + F + 1e-200));
}

////////////////////////////////////////////////////////////////////////////////

double c_eds :: get_F_t_error()
{
  // returns measure of error due to temporal resolution
  double F = get_total_flux();
  
  double Flores = 0.;
  int iur;
  double h = (log(ur_max) - log(ur_min)) / (double) ur_rays;

  for (iur = 0; iur < ur_rays / 5; iur++)
    Flores += 5 * h / 288. * (
      19. * get_F_annulus_lores_t(iur * 5) +
      75. * get_F_annulus_lores_t(iur * 5 + 1) +
      50. * get_F_annulus_lores_t(iur * 5 + 2) +
      50. * get_F_annulus_lores_t(iur * 5 + 3) +
      75. * get_F_annulus_lores_t(iur * 5 + 4) +
      19. * get_F_annulus_lores_t(iur * 5 + 5));

  Flores = (2.0 * Flores) * (1.0 + p_Obs->z) / (p_Obs->dL * p_Obs->dL);

  return 2. * fabs((Flores - F) / (Flores + F + 1e-200));
}

////////////////////////////////////////////////////////////////////////////////
// BEGIN CODE NOT PUBLIC

void c_eds :: set_R()
{
  int iur;
  double df;
  
  R_50 = -1.;
  R_75 = -1.;
  R_95 = -1.;
  R_99 = -1.;
  R_100 = -1;
  
  double F_total = 0.0, f = 0.0; // 'f' is fractional flux

  // get total
  for (iur = 0; iur <= ur_rays; iur++)
    F_total += get_F_annulus(iur);
  
  // get fractional results
  R_99 = -1.0; R_95 = -1.0; R_75 = -1.0; R_50 = -1.0;
  
  for (iur = 0; iur <= ur_rays; iur++)
  {
    df = get_F_annulus(iur);
    f += df;
    if (f > 0.5 * F_total and R_50 < 0.0) R_50 = ray[iur][0].ur;
    if (f > 0.75 * F_total and R_75 < 0.0) R_75 = ray[iur][0].ur;
    if (f > 0.95 * F_total and R_95 < 0.0) R_95 = ray[iur][0].ur;
    if (f > 0.99 * F_total and R_99 < 0.0) R_99 = ray[iur][0].ur;
    if (df > 0.) R_100 = ray[iur][0].ur;
  }
}

// END CODE NOT PUBLIC
////////////////////////////////////////////////////////////////////////////////

void c_eds :: reset()
{
  int iur, iuphi;
  double dlnur, duphi;
  
  //----------------------------------------------------------------------------
  // set coordinates assumes that lnur_max and lnur_min are set
  duphi = (PI - 0.0) / (double) uphi_rays;
  dlnur = (log(ur_max) - log(ur_min)) / (double) ur_rays;

  // set unit vectors on the EDS plane in the frame of the grid, 
  // assuming theta_obs related variables are set
  eux[0] = p_Obs->costheta; eux[1] = 0.0; eux[2] = -p_Obs->sintheta;
  euy[0] = 0.0; euy[1] = 1.0; euy[2] = 0.0;

  for (iur = 0; iur <= ur_rays; iur++)
  {
    for (iuphi = 0; iuphi <= uphi_rays; iuphi++)
    {
      ray[iur][iuphi].ur = exp(log(ur_min) + iur * dlnur);
      ray[iur][iuphi].uphi = iuphi * duphi;

      ray[iur][iuphi].ux = ray[iur][iuphi].ur * cos(ray[iur][iuphi].uphi); 
      ray[iur][iuphi].uy = ray[iur][iuphi].ur * sin(ray[iur][iuphi].uphi);
      
      // reset radiation
      ray[iur][iuphi].em = 0.0;
      ray[iur][iuphi].ab = 0.0;
      
      #if LOCAL_SELF_ABSORPTION_ == DISABLED_

        ray[iur][iuphi].emdr = 0.0;
        ray[iur][iuphi].abdr = 0.0;

        ray[iur][iuphi].emdr_lores = 0.0;
        ray[iur][iuphi].abdr_lores = 0.0;
      
      #else
      
        ray[iur][iuphi].I = 0.0;
        ray[iur][iuphi].I_lores = 0.0;
        
      #endif
      
    }
  }
  
  F = 0.;

  return;
}
