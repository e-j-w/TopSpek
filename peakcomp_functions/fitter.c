#include "peakcomp_functions.h"

//function computes background coefficients and scaling factors
//by analytically minimizing chisq for the expression:
//chisq=sum_i[(meas_i - A - B*i - scaleFactor_1*sim_1i - scaleFactor_2*sim_2i - ...)^2 / meas_i]
void computeBackgroundandScaling(pc_par * par, fit_par * fpar)
{

  long double m_sum,s_sum[NSIMDATA],ss_sum[NSIMDATA][NSIMDATA],ms_sum[NSIMDATA],mi_sum,mii_sum,si_sum[NSIMDATA],sii_sum[NSIMDATA],i_sum,ii_sum,iii_sum,iiii_sum,sum1; //sums needed to construct system of equations
  long double ind;
  double **fittedScaleFactor=allocateArrayD2(NSIMDATA,NSPECT);
  int i,j,k,l;
  lin_eq_type linEq;
  printf("\n");
  
  if(par->numFittedSimData>0)
    {
      for (i=0;i<par->numSpectra;i++)
        {
          //initialize all sums to 0
          m_sum=0.;
          mi_sum=0.;
          mii_sum=0.;
          i_sum=0.;
          ii_sum=0.;
          iii_sum=0.;
          iiii_sum=0.;
          sum1=0.;
          memset(s_sum,0,sizeof(s_sum));
          memset(ms_sum,0,sizeof(ms_sum));
          memset(si_sum,0,sizeof(si_sum));
          memset(sii_sum,0,sizeof(sii_sum));
          memset(ss_sum,0,sizeof(ss_sum));
          
          //construct sums
          for (j=par->startCh[i];j<=par->endCh[i];j++)
            if(expHist[par->spectrum[i]][j]!=0)
              {
                m_sum+=fittedExpHist[par->spectrum[i]][j]/((double)expHist[par->spectrum[i]][j]);
                for (k=0;k<par->numFittedSimData;k++)
                  {
                    ms_sum[k]+=(double)fittedExpHist[par->spectrum[i]][j]*(double)fittedSimHist[k][par->spectrum[i]][j]/((double)expHist[par->spectrum[i]][j]);//cast to double needed to prevent overflow
                    for (l=0;l<par->numFittedSimData;l++)
                      ss_sum[k][l]+=(double)fittedSimHist[k][par->spectrum[i]][j]*(double)fittedSimHist[l][par->spectrum[i]][j]/((double)expHist[par->spectrum[i]][j]);
                  }
              }
          if(par->fitAddBackground[i]>=1)
            for (j=par->startCh[i];j<=par->endCh[i];j++)
              if(expHist[par->spectrum[i]][j]!=0)
                {
                  ind=(long double)j;  
                  mi_sum+=fittedExpHist[par->spectrum[i]][j]*ind/((double)expHist[par->spectrum[i]][j]);
                  i_sum+=ind/((double)expHist[par->spectrum[i]][j]);
                  ii_sum+=ind*ind/((double)expHist[par->spectrum[i]][j]);
                  sum1+=1./((double)expHist[par->spectrum[i]][j]);
                  for (k=0;k<par->numFittedSimData;k++)
                    {
                      s_sum[k]+=fittedSimHist[k][par->spectrum[i]][j]/((double)expHist[par->spectrum[i]][j]);
                      si_sum[k]+=fittedSimHist[k][par->spectrum[i]][j]*ind/((double)expHist[par->spectrum[i]][j]);
                    }
                }
          if(par->fitAddBackground[i]>=2)
            for (j=par->startCh[i];j<=par->endCh[i];j++)
              if(expHist[par->spectrum[i]][j]!=0)
                {
                  ind=(long double)j;
                  mii_sum+=fittedExpHist[par->spectrum[i]][j]*ind*ind/((double)expHist[par->spectrum[i]][j]);
                  iii_sum+=ind*ind*ind/((double)expHist[par->spectrum[i]][j]);
                  iiii_sum+=ind*ind*ind*ind/((double)expHist[par->spectrum[i]][j]);
                  for (k=0;k<par->numFittedSimData;k++)
                    sii_sum[k]+=fittedSimHist[k][par->spectrum[i]][j]*ind*ind/((double)expHist[par->spectrum[i]][j]);
                }
          
          //construct system of equations (matrix/vector entries) 
          if(par->fitAddBackground[i]==0)
            {
              linEq.dim=par->numFittedSimData;
              for (j=0;j<par->numFittedSimData;j++)
                for (k=0;k<par->numFittedSimData;k++)
                  linEq.matrix[j][k]=ss_sum[j][k];
              for (j=0;j<par->numFittedSimData;j++)
                linEq.vector[j]=ms_sum[j];
            }
          else if(par->fitAddBackground[i]==1)
            {
              linEq.dim=par->numFittedSimData+2;
              
              //top-left 4 entries
              linEq.matrix[0][0]=sum1;
              linEq.matrix[0][1]=i_sum;
              linEq.matrix[1][0]=i_sum;
              linEq.matrix[1][1]=ii_sum;
              
              //regular simulated data entires (bottom-right)
              for (j=0;j<par->numFittedSimData;j++)
                for (k=0;k<par->numFittedSimData;k++)
                  linEq.matrix[2+j][2+k]=ss_sum[j][k];
              
              //remaining entires
              for (j=0;j<par->numFittedSimData;j++)
                {     
                  linEq.matrix[0][2+j]=s_sum[j];
                  linEq.matrix[1][2+j]=si_sum[j];
                  linEq.matrix[2+j][0]=s_sum[j];
                  linEq.matrix[2+j][1]=si_sum[j];
                }
              
              linEq.vector[0]=m_sum;
              linEq.vector[1]=mi_sum;
              for (j=0;j<par->numFittedSimData;j++)
                linEq.vector[j+2]=ms_sum[j];
            }
          else if(par->fitAddBackground[i]==2)
            {
              linEq.dim=par->numFittedSimData+3;
              
              //top-left 9 entries
              linEq.matrix[0][0]=sum1;
              linEq.matrix[0][1]=i_sum;
              linEq.matrix[0][2]=ii_sum;
              linEq.matrix[1][0]=i_sum;
              linEq.matrix[1][1]=ii_sum;
              linEq.matrix[1][2]=iii_sum;
              linEq.matrix[2][0]=ii_sum;
              linEq.matrix[2][1]=iii_sum;
              linEq.matrix[2][2]=iiii_sum;
              
              //regular simulated data entires (bottom-right)
              for (j=0;j<par->numFittedSimData;j++)
                for (k=0;k<par->numFittedSimData;k++)
                  linEq.matrix[3+j][3+k]=ss_sum[j][k];
              
              //remaining entires
              for (j=0;j<par->numFittedSimData;j++)
                {     
                  linEq.matrix[0][3+j]=s_sum[j];
                  linEq.matrix[1][3+j]=si_sum[j];
                  linEq.matrix[2][3+j]=sii_sum[j];
                  linEq.matrix[3+j][0]=s_sum[j];
                  linEq.matrix[3+j][1]=si_sum[j];
                  linEq.matrix[3+j][2]=sii_sum[j];
                }
              
              linEq.vector[0]=m_sum;
              linEq.vector[1]=mi_sum;
              linEq.vector[2]=mii_sum;
              for (j=0;j<par->numFittedSimData;j++)
                linEq.vector[j+3]=ms_sum[j];
            }
          
          //solve system of equations and assign values
          if(!(solve_lin_eq(&linEq)==1))
            {
              if(m_sum==0)
                printf("ERROR: Experiment data (spectrum %i) has no entries in the specified fitting region!\n",par->spectrum[i]);
              else
                printf("ERROR: Could not determine background and scaling parameters!\n");
              exit(-1);
            }
          
          if(par->fitAddBackground[i]==0)
            {
              fpar->bgA[par->spectrum[i]]=0.;
              fpar->bgB[par->spectrum[i]]=0.;
              fpar->bgC[par->spectrum[i]]=0.;
              for (j=0;j<par->numFittedSimData;j++)
                fittedScaleFactor[j][par->spectrum[i]]=linEq.solution[j];
            }
          else if(par->fitAddBackground[i]==1)
            {
              fpar->bgA[par->spectrum[i]]=linEq.solution[0];
              fpar->bgB[par->spectrum[i]]=linEq.solution[1];
              fpar->bgC[par->spectrum[i]]=0.;
              for (j=0;j<par->numFittedSimData;j++)
                fittedScaleFactor[j][par->spectrum[i]]=linEq.solution[j+2];
            }
          else if(par->fitAddBackground[i]==2)
            {
              fpar->bgA[par->spectrum[i]]=linEq.solution[0];
              fpar->bgB[par->spectrum[i]]=linEq.solution[1];
              fpar->bgC[par->spectrum[i]]=linEq.solution[2];
              for (j=0;j<par->numFittedSimData;j++)
                fittedScaleFactor[j][par->spectrum[i]]=linEq.solution[j+3];
            }

        }
    }
  else
    printf("NOTE: All scaling parameters are fixed, no chisq minimzation was performed.\n\n");
    
  //generate scaling factors for all spectra, including those that weren't fitted
  int fd=0;//counter for number of datasets which have fit (not fixed amplitude)
  int ld=-1;//index of the last dataset for which a scaling factor was determined
  for (i=0;i<par->numSimData;i++)
    {
      if(par->simDataFixedAmp[i]==0)//data was fit
        {
          for (j=0;j<par->numSpectra;j++)
            fpar->scaleFactor[i][par->spectrum[j]]=fittedScaleFactor[fd][par->spectrum[j]];
          fd++;
          ld=i;
        }
      else if(par->simDataFixedAmp[i]==2)//data is scaled relative to the previous fit data
        {
          if(ld>=0)//has a previous dataset been fit?
            for (j=0;j<par->numSpectra;j++)
              fpar->scaleFactor[i][par->spectrum[j]]=par->simDataFixedAmpValue[i]*fpar->scaleFactor[ld][par->spectrum[j]];
          ld=i;
        }
      else//data wasn't fit
        {
          for (j=0;j<par->numSpectra;j++)
            fpar->scaleFactor[i][par->spectrum[j]]=par->simDataFixedAmpValue[i];
          ld=i;
        }
    }
  //check for fixed background and generate background parameters if needed
  for (i=0;i<par->numSimData;i++)
    if(par->fixBG[i]==1)
      {
        if(par->addBackground>=1)
          {
            fpar->bgA[par->spectrum[i]]=par->fixedBGPar[i][0];
            fpar->bgB[par->spectrum[i]]=par->fixedBGPar[i][1];
          }
        if(par->addBackground>=2)
          {
            fpar->bgC[par->spectrum[i]]=par->fixedBGPar[i][2];
          }
      }
  
  free(fittedScaleFactor);
  
  //print fit data
  printf("FIT DATA\n--------\n");
  for (i=0;i<par->numSpectra;i++)
    {
      if(par->addBackground==0)
        printf("Spectrum %i, channel %i to %i:\n",par->spectrum[i],par->startCh[i],par->endCh[i]);
      else if((par->addBackground==1)&&(par->fitAddBackground[i]==par->addBackground))
        printf("Spectrum %i, channel %i to %i:\nFit linear background of form [A + B*channel],\nA = %0.5LE, B = %0.5LE\n",par->spectrum[i],par->startCh[i],par->endCh[i],fpar->bgA[par->spectrum[i]],fpar->bgB[par->spectrum[i]]);
      else if(par->addBackground==1)
        printf("Spectrum %i, channel %i to %i:\nUsing linear background of form [A + B*channel],\nA = %0.5LE [FIXED], B = %0.5LE [FIXED]\n",par->spectrum[i],par->startCh[i],par->endCh[i],fpar->bgA[par->spectrum[i]],fpar->bgB[par->spectrum[i]]);
      else if((par->addBackground==2)&&(par->fitAddBackground[i]==par->addBackground))
        printf("Spectrum %i, channel %i to %i:\nFit quadratic background of form [A + B*channel + C*(channel^2)],\nA = %0.5LE, B = %0.5LE, C = %0.5LE\n",par->spectrum[i],par->startCh[i],par->endCh[i],fpar->bgA[par->spectrum[i]],fpar->bgB[par->spectrum[i]],fpar->bgC[par->spectrum[i]]);
      else if(par->addBackground==2)
        printf("Spectrum %i, channel %i to %i:\nUsing quadratic background of form [A + B*channel + C*(channel^2)],\nA = %0.5LE [FIXED], B = %0.5LE [FIXED], C = %0.5LE [FIXED]\n",par->spectrum[i],par->startCh[i],par->endCh[i],fpar->bgA[par->spectrum[i]],fpar->bgB[par->spectrum[i]],fpar->bgC[par->spectrum[i]]);
      for (j=0;j<par->numSimData;j++)
        if(par->simDataFixedAmp[j]==0)
          printf("Scaling factor for data from file %s: %f\n",par->simDataName[j],fpar->scaleFactor[j][par->spectrum[i]]);
        else
          printf("Scaling factor for data from file %s: %f [FIXED]\n",par->simDataName[j],fpar->scaleFactor[j][par->spectrum[i]]);
      printf("\n");
    }

}
