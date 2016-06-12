/*======================================================================

  This file is part of the elastix software.

  Copyright (c) Erasmus MC, Rotterdam. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/

#ifndef PCAMETRIC_F_HXX
#define PCAMETRIC_F_HXX
#include "itkPCAMetric_F.h"
#include "itkMersenneTwisterRandomVariateGenerator.h"
#include "vnl/algo/vnl_matrix_update.h"
#include "itkImage.h"
#include "vnl/algo/vnl_svd.h"
#include "vnl/vnl_trace.h"
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include <numeric>
#include <fstream>

namespace itk
{
/**
 * ******************* Constructor *******************
 */

  template <class TFixedImage, class TMovingImage>
    PCAMetric<TFixedImage,TMovingImage>
      ::PCAMetric():
        m_SubtractMean( false ),
        m_TransformIsStackTransform( false ),
        m_NumEigenValues( 6 )
  {
    this->SetUseImageSampler( true );
    this->SetUseFixedImageLimiter( false );
    this->SetUseMovingImageLimiter( false );
  } // end constructor

 /**
 * ******************* Initialize *******************
 */

  template <class TFixedImage, class TMovingImage>
    void
   PCAMetric<TFixedImage,TMovingImage>
    ::Initialize(void) throw ( ExceptionObject )
  {

        /** Initialize transform, interpolator, etc. */
    Superclass::Initialize();

    /** Retrieve slowest varying dimension and its size. */
    const unsigned int lastDim = this->GetFixedImage()->GetImageDimension() - 1;
    const unsigned int G = this->GetFixedImage()->GetLargestPossibleRegion().GetSize( lastDim );

    if( this->m_NumEigenValues > G )
    {
       std::cerr << "ERROR: Number of eigenvalues is larger than number of images. Maximum number of eigenvalues equals: "
                 << G << std::endl;
    }
  } // end Initialize


/**
 * ******************* PrintSelf *******************
 */

  template < class TFixedImage, class TMovingImage>
    void
    PCAMetric<TFixedImage,TMovingImage>
    ::PrintSelf(std::ostream& os, Indent indent) const
  {
    Superclass::PrintSelf( os, indent );

  } // end PrintSelf


/**
 * ******************* SampleRandom *******************
 */

  template < class TFixedImage, class TMovingImage>
  void
     PCAMetric<TFixedImage,TMovingImage>
    ::SampleRandom ( const int n, const int m, std::vector<int> & numbers ) const
  {
    /** Empty list of last dimension positions. */
    numbers.clear();

    /** Initialize random number generator. */
    Statistics::MersenneTwisterRandomVariateGenerator::Pointer randomGenerator = Statistics::MersenneTwisterRandomVariateGenerator::GetInstance();

    /** Sample additional at fixed timepoint. */
    for ( unsigned int i = 0; i < m_NumAdditionalSamplesFixed; ++i )
    {
      numbers.push_back( this->m_ReducedDimensionIndex );
    }

    /** Get n random samples. */
    for ( int i = 0; i < n; ++i )
    {
      int randomNum = 0;
      do
      {
        randomNum = static_cast<int>( randomGenerator->GetVariateWithClosedRange( m ) );
      } while ( find( numbers.begin(), numbers.end(), randomNum ) != numbers.end() );
      numbers.push_back( randomNum );
    }
  } // end SampleRandom

/**
 * *************** EvaluateTransformJacobianInnerProduct ****************
 */

  template < class TFixedImage, class TMovingImage >
    void
    PCAMetric<TFixedImage,TMovingImage>
    ::EvaluateTransformJacobianInnerProduct(
    const TransformJacobianType & jacobian,
    const MovingImageDerivativeType & movingImageDerivative,
    DerivativeType & imageJacobian ) const
  {
    typedef typename TransformJacobianType::const_iterator JacobianIteratorType;
    typedef typename DerivativeType::iterator              DerivativeIteratorType;
    JacobianIteratorType jac = jacobian.begin();
    imageJacobian.Fill( 0.0 );
    const unsigned int sizeImageJacobian = imageJacobian.GetSize();
    for ( unsigned int dim = 0; dim < FixedImageDimension; dim++ )
    {
      const double imDeriv = movingImageDerivative[ dim ];
      DerivativeIteratorType imjac = imageJacobian.begin();

      for ( unsigned int mu = 0; mu < sizeImageJacobian; mu++ )
      {
        (*imjac) += (*jac) * imDeriv;
        ++imjac;
        ++jac;
      }
    }
  } // end EvaluateTransformJacobianInnerProduct

    /**
 * ******************* GetValue *******************
 */

  template <class TFixedImage, class TMovingImage>
    typename PCAMetric<TFixedImage,TMovingImage>::MeasureType
    PCAMetric<TFixedImage,TMovingImage>
    ::GetValue( const TransformParametersType & parameters ) const
  {
    itkDebugMacro( "GetValue( " << parameters << " ) " );

    /** Make sure the transform parameters are up to date. */
    this->SetTransformParameters( parameters );

    /** Initialize some variables */
    this->m_NumberOfPixelsCounted = 0;
    MeasureType measure = NumericTraits< MeasureType >::Zero;

    /** Update the imageSampler and get a handle to the sample container. */
    this->GetImageSampler()->Update();
    ImageSampleContainerPointer sampleContainer = this->GetImageSampler()->GetOutput();

    /** Create iterator over the sample container. */
    typename ImageSampleContainerType::ConstIterator fiter;
    typename ImageSampleContainerType::ConstIterator fbegin = sampleContainer->Begin();
    typename ImageSampleContainerType::ConstIterator fend = sampleContainer->End();

    /** Retrieve slowest varying dimension and its size. */
    const unsigned int lastDim = this->GetFixedImage()->GetImageDimension() - 1;
    const unsigned int G = this->GetFixedImage()->GetLargestPossibleRegion().GetSize( lastDim );

    typedef vnl_matrix< RealType > MatrixType;

    /** The rows of the ImageSampleMatrix contain the samples of the images of the stack */
    unsigned int NumberOfSamples = sampleContainer->Size();
    MatrixType datablock( NumberOfSamples, G );

    /** Initialize dummy loop variable */
    unsigned int pixelIndex = 0;

    /** Initialize image sample matrix . */
    datablock.fill( itk::NumericTraits< RealType>::Zero );

    for ( fiter = fbegin; fiter != fend; ++fiter )
    {
        /** Read fixed coordinates. */
        FixedImagePointType fixedPoint = (*fiter).Value().m_ImageCoordinates;

        /** Transform sampled point to voxel coordinates. */
        FixedImageContinuousIndexType voxelCoord;
        this->GetFixedImage()->TransformPhysicalPointToContinuousIndex( fixedPoint, voxelCoord );

        unsigned int numSamplesOk = 0;

        /** Loop over t */
        for ( unsigned int d = 0; d < G; ++d )
        {
            /** Initialize some variables. */
            RealType movingImageValue;
            MovingImagePointType mappedPoint;

            /** Set fixed point's last dimension to lastDimPosition. */
            voxelCoord[ lastDim ] = d;

            /** Transform sampled point back to world coordinates. */
            this->GetFixedImage()->TransformContinuousIndexToPhysicalPoint( voxelCoord, fixedPoint );

            /** Transform point and check if it is inside the B-spline support region. */
            bool sampleOk = this->TransformPoint( fixedPoint, mappedPoint );

            /** Check if point is inside mask. */
            if ( sampleOk )
            {
                sampleOk = this->IsInsideMovingMask( mappedPoint );
            }

            if ( sampleOk )
            {
                sampleOk = this->EvaluateMovingImageValueAndDerivative(
                            mappedPoint, movingImageValue, 0 );
            }

            if( sampleOk )
            {
                numSamplesOk++;
                datablock( pixelIndex, d ) = movingImageValue;
            }

        } /** end loop over t */

        if( numSamplesOk == G )
        {
            pixelIndex++;
            this->m_NumberOfPixelsCounted++;
        }

    }/** end first loop over image sample container */

    /** Check if enough samples were valid. */
    this->CheckNumberOfSamples(NumberOfSamples, this->m_NumberOfPixelsCounted );
    unsigned int N = this->m_NumberOfPixelsCounted;
    MatrixType A( datablock.extract( N, G ) );

    /** Calculate mean of from columns */
    vnl_vector< RealType > mean( G );
    mean.fill( NumericTraits< RealType >::Zero );
    for( int i = 0; i < N; i++ )
    {
        for( int j = 0; j < G; j++)
        {
            mean(j) += A(i,j);
        }
    }
    mean /= RealType(N);

    MatrixType Amm( N, G );
    Amm.fill( NumericTraits< RealType >::Zero );

    for (int i = 0; i < N; i++ )
    {
        for(int j = 0; j < G; j++)
        {
            Amm(i,j) = A(i,j)-mean(j);
        }
    }

    /** Compute covariancematrix C */
    MatrixType C( Amm.transpose()*Amm );
    C /=  static_cast< RealType > ( RealType(N) - 1.0 );

    vnl_diag_matrix< RealType > S( G );
    S.fill( NumericTraits< RealType >::Zero );
    for( int j = 0; j < G; j++)
    {
        S(j,j) = 1.0/sqrt(C(j,j));
    }

    /** Compute correlation matrix K */
    MatrixType K(S*C*S);

    /** Compute first eigenvalue and eigenvector of K */
    vnl_symmetric_eigensystem< RealType > eig( K );

    const unsigned int L = this->m_NumEigenValues;

    RealType sumEigenValuesUsed = itk::NumericTraits< RealType >::Zero;
    for(unsigned int i = 1; i < L+1; i++)
    {
        sumEigenValuesUsed += eig.get_eigenvalue(G - i);
    }

    measure = G - sumEigenValuesUsed;

    /** Return the measure value. */
    return measure;

 } // end GetValue

/**
 * ******************* GetDerivative *******************
 */

  template < class TFixedImage, class TMovingImage>
    void
    PCAMetric<TFixedImage,TMovingImage>
    ::GetDerivative( const TransformParametersType & parameters,
    DerivativeType & derivative ) const
  {
    /** When the derivative is calculated, all information for calculating
     * the metric value is available. It does not cost anything to calculate
     * the metric value now. Therefore, we have chosen to only implement the
     * GetValueAndDerivative(), supplying it with a dummy value variable. */
    MeasureType dummyvalue = NumericTraits< MeasureType >::Zero;

    this->GetValueAndDerivative(parameters, dummyvalue, derivative);

  } // end GetDerivative

/**
 * ******************* GetValueAndDerivative *******************
 */

    template <class TFixedImage, class TMovingImage>
    void
    PCAMetric<TFixedImage,TMovingImage>
    ::GetValueAndDerivative( const TransformParametersType & parameters,
    MeasureType& value, DerivativeType& derivative ) const
  {
    itkDebugMacro("GetValueAndDerivative( " << parameters << " ) ");
    /** Define derivative and Jacobian types. */
    typedef typename DerivativeType::ValueType        DerivativeValueType;
    typedef typename TransformJacobianType::ValueType TransformJacobianValueType;

    /** Initialize some variables */
    const unsigned int P = this->GetNumberOfParameters();
    this->m_NumberOfPixelsCounted = 0;
    MeasureType measure = NumericTraits< MeasureType >::Zero;
    derivative = DerivativeType( P );
    derivative.Fill( NumericTraits< DerivativeValueType >::Zero );

    /** Make sure the transform parameters are up to date. */
    this->SetTransformParameters( parameters );

    /** Update the imageSampler and get a handle to the sample container. */
    this->GetImageSampler()->Update();
    ImageSampleContainerPointer sampleContainer = this->GetImageSampler()->GetOutput();

    /** Create iterator over the sample container. */
    typename ImageSampleContainerType::ConstIterator fiter;
    typename ImageSampleContainerType::ConstIterator fbegin = sampleContainer->Begin();
    typename ImageSampleContainerType::ConstIterator fend = sampleContainer->End();

    /** Retrieve slowest varying dimension and its size. */
    const unsigned int lastDim = this->GetFixedImage()->GetImageDimension() - 1;
    const unsigned int G = this->GetFixedImage()->GetLargestPossibleRegion().GetSize( lastDim );

    typedef vnl_matrix< RealType >                  MatrixType;
    typedef vnl_matrix< DerivativeValueType > DerivativeMatrixType;

    std::vector< FixedImagePointType > SamplesOK;

    /** The rows of the ImageSampleMatrix contain the samples of the images of the stack */
    unsigned int NumberOfSamples = sampleContainer->Size();
    MatrixType datablock( NumberOfSamples, G );

    /** Initialize dummy loop variables */
    unsigned int pixelIndex = 0;

    /** Initialize image sample matrix . */
    datablock.fill( itk::NumericTraits< RealType >::Zero );

    for ( fiter = fbegin; fiter != fend; ++fiter )
    {
        /** Read fixed coordinates. */
        FixedImagePointType fixedPoint = (*fiter).Value().m_ImageCoordinates;

        /** Transform sampled point to voxel coordinates. */
        FixedImageContinuousIndexType voxelCoord;
        this->GetFixedImage()->TransformPhysicalPointToContinuousIndex( fixedPoint, voxelCoord );

        unsigned int numSamplesOk = 0;

        /** Loop over t */
        for ( unsigned int d = 0; d < G; ++d )
        {
            /** Initialize some variables. */
            RealType movingImageValue;
            MovingImagePointType mappedPoint;

            /** Set fixed point's last dimension to lastDimPosition. */
            voxelCoord[ lastDim ] = d;

            /** Transform sampled point back to world coordinates. */
            this->GetFixedImage()->TransformContinuousIndexToPhysicalPoint( voxelCoord, fixedPoint );

            /** Transform point and check if it is inside the B-spline support region. */
            bool sampleOk = this->TransformPoint( fixedPoint, mappedPoint );
            /** Check if point is inside mask. */
            if( sampleOk )
            {
                sampleOk = this->IsInsideMovingMask( mappedPoint );
            }

            if( sampleOk )

            {
                sampleOk = this->EvaluateMovingImageValueAndDerivative(
                            mappedPoint, movingImageValue, 0 );
            }

            if( sampleOk )
            {
                numSamplesOk++;
                datablock( pixelIndex, d ) = movingImageValue;
            }// end if sampleOk

        } // end loop over t
        if( numSamplesOk == G )
        {
            SamplesOK.push_back(fixedPoint);
            pixelIndex++;
            this->m_NumberOfPixelsCounted++;
        }

    }/** end first loop over image sample container */

    /** Check if enough samples were valid. */
    this->CheckNumberOfSamples( sampleContainer->Size(), this->m_NumberOfPixelsCounted );
    unsigned int N = this->m_NumberOfPixelsCounted;

    MatrixType A( datablock.extract( N, G ) );

    /** Calculate mean of from columns */
    vnl_vector< RealType > mean( G );
    mean.fill( NumericTraits< RealType >::Zero );
    for( int i = 0; i < N; i++ )
    {
        for( int j = 0; j < G; j++)
        {
            mean(j) += A(i,j);
        }
    }
    mean /= RealType(N);

    /** Calculate standard deviation from columns */
    MatrixType Amm( N, G );
    Amm.fill( NumericTraits< RealType >::Zero );
    for( int i = 0; i < N; i++ )
    {
        for( int j = 0; j < G; j++)
        {
            Amm(i,j) = A(i,j)-mean(j);
        }
    }

    /** Compute covariancematrix C */
    MatrixType Atmm = Amm.transpose();
    MatrixType C( Atmm*Amm );
    C /=  static_cast< RealType > ( RealType(N) - 1.0 );

    vnl_diag_matrix< RealType > S( G );
    S.fill( NumericTraits< RealType >::Zero );
    for( int j = 0; j < G; j++)
    {
        S(j,j) = 1.0/sqrt(C(j,j));
    }

    MatrixType K(S*C*S);

    /** Compute first eigenvalue and eigenvector of K */
    vnl_symmetric_eigensystem< RealType > eig( K );

    const unsigned int L = this->m_NumEigenValues;

    RealType sumEigenValuesUsed = itk::NumericTraits< RealType >::Zero;
    for(unsigned int i = 1; i < L+1; i++)
    {
        sumEigenValuesUsed += eig.get_eigenvalue(G - i);
    }

    MatrixType eigenVectorMatrix( G, L );
    for(unsigned int i = 1; i < L+1; i++)
    {
        eigenVectorMatrix.set_column(i-1, (eig.get_eigenvector(G - i)).normalize() );
    }

    MatrixType eigenVectorMatrixTranspose( eigenVectorMatrix.transpose() );

    /** Create variables to store intermediate results in. */
    TransformJacobianType jacobian;
    DerivativeType imageJacobian( this->m_AdvancedTransform->GetNumberOfNonZeroJacobianIndices() );
    std::vector<NonZeroJacobianIndicesType> nzjis( G, NonZeroJacobianIndicesType() );

    /** Sub components of metric derivative */
    vnl_diag_matrix< DerivativeValueType > dSdmu_part1( G );

    /** initialize */
    dSdmu_part1.fill( itk::NumericTraits< DerivativeValueType >::Zero );

    for(unsigned int d = 0; d < G; d++)
    {
        double S_sqr = S(d,d) * S(d,d);
        double S_qub = S_sqr * S(d,d);
        dSdmu_part1(d, d) = -S_qub;
    }

    DerivativeMatrixType vSAtmm( eigenVectorMatrixTranspose*S*Atmm );
    DerivativeMatrixType CSv( C*S*eigenVectorMatrix );
    DerivativeMatrixType Sv( S*eigenVectorMatrix );
    DerivativeMatrixType vdSdmu_part1( eigenVectorMatrixTranspose*dSdmu_part1 );

    /** Second loop over fixed image samples. */
//#pragma omp parallel shared(vSAtmm, CSv, Sv, vdSdmu_part1, derivative, jacobian, imageJacobian, nzjis)
{
//    #pragma omp single nowait
    {
        for ( pixelIndex = 0; pixelIndex < SamplesOK.size(); ++pixelIndex )
//        #pragma omp task
        {
            /** Read fixed coordinates. */
            FixedImagePointType fixedPoint = SamplesOK[ pixelIndex ];

            /** Transform sampled point to voxel coordinates. */
            FixedImageContinuousIndexType voxelCoord;
            this->GetFixedImage()->TransformPhysicalPointToContinuousIndex( fixedPoint, voxelCoord );

            for ( unsigned int d = 0; d < G; ++d )
            {
                /** Initialize some variables. */
                RealType movingImageValue;
                MovingImagePointType mappedPoint;
                MovingImageDerivativeType movingImageDerivative;


                /** Set fixed point's last dimension to lastDimPosition. */
                voxelCoord[ lastDim ] = d;

                /** Transform sampled point back to world coordinates. */
                this->GetFixedImage()->TransformContinuousIndexToPhysicalPoint( voxelCoord, fixedPoint );
                this->TransformPoint( fixedPoint, mappedPoint );

                this->EvaluateMovingImageValueAndDerivative(
                            mappedPoint, movingImageValue, &movingImageDerivative );

                /** Get the TransformJacobian dT/dmu */
                this->EvaluateTransformJacobian( fixedPoint, jacobian, nzjis[ d ] );

                /** Compute the innerproduct (dM/dx)^T (dT/dmu). */
                this->EvaluateTransformJacobianInnerProduct(
                            jacobian, movingImageDerivative, imageJacobian );

                /** Store values. */
                /* build metric derivative components */
                for( unsigned int p = 0; p < nzjis[ d ].size(); ++p)
                {
                    double temp = 0;
                    for(unsigned int z = 0; z < L; z++)
                    {
                        temp += vSAtmm[ z ][ pixelIndex ] * imageJacobian[ p ] * Sv[ d ][ z ] +
                                vdSdmu_part1[ z ][ d ] * Atmm[ d ][ pixelIndex ] * imageJacobian[ p ] * CSv[ d ][ z ];
                    }//end loop over eigenvalues
//                    #pragma omp critical (derivative)
                    derivative[ nzjis[ d ][ p ] ] += temp;
                }//end loop over non-zero jacobian indices

            }//end loop over last dimension

        } // end second for loop over sample container
    }
}

    derivative *= -(2.0/(DerivativeValueType(N) - 1.0)); //normalize
    measure = G - sumEigenValuesUsed;

    /** Subtract mean from derivative elements. */
    if( this->m_SubtractMean )
    {
      if( ! this->m_TransformIsStackTransform )
      {
        /** Update derivative per dimension.
         * Parameters are ordered xxxxxxx yyyyyyy zzzzzzz ttttttt and
         * per dimension xyz.
         */
        const unsigned int lastDimGridSize = this->m_GridSize[ lastDim ];
        const unsigned int numParametersPerDimension
          = this->GetNumberOfParameters() / this->GetMovingImage()->GetImageDimension();
        const unsigned int numControlPointsPerDimension = numParametersPerDimension / lastDimGridSize;
        DerivativeType mean ( numControlPointsPerDimension );
        for ( unsigned int d = 0; d < this->GetMovingImage()->GetImageDimension(); ++d )
        {
          /** Compute mean per dimension. */
          mean.Fill( 0.0 );
          const unsigned int starti = numParametersPerDimension * d;
          for ( unsigned int i = starti; i < starti + numParametersPerDimension; ++i )
          {
            const unsigned int index = i % numControlPointsPerDimension;
            mean[ index ] += derivative[ i ];
          }
          mean /= static_cast< RealType >( lastDimGridSize );

          /** Update derivative for every control point per dimension. */
          for ( unsigned int i = starti; i < starti + numParametersPerDimension; ++i )
          {
            const unsigned int index = i % numControlPointsPerDimension;
            derivative[ i ] -= mean[ index ];
          }
        }
      }
      else
      {
        /** Update derivative per dimension.
         * Parameters are ordered x0x0x0y0y0y0z0z0z0x1x1x1y1y1y1z1z1z1 with
         * the number the time point index.
         */
        const unsigned int numParametersPerLastDimension = this->GetNumberOfParameters() / G;
        DerivativeType mean ( numParametersPerLastDimension );
        mean.Fill( 0.0 );

        /** Compute mean per control point. */
        for ( unsigned int t = 0; t < G; ++t )
        {
          const unsigned int startc = numParametersPerLastDimension * t;
          for ( unsigned int c = startc; c < startc + numParametersPerLastDimension; ++c )
          {
            const unsigned int index = c % numParametersPerLastDimension;
            mean[ index ] += derivative[ c ];
          }
        }
        mean /= static_cast< RealType >( G );

        /** Update derivative per control point. */
        for ( unsigned int t = 0; t < G; ++t )
        {
          const unsigned int startc = numParametersPerLastDimension * t;
          for ( unsigned int c = startc; c < startc + numParametersPerLastDimension; ++c )
          {
            const unsigned int index = c % numParametersPerLastDimension;
            derivative[ c ] -= mean[ index ];
          }
        }
      }
    }

    /** Return the measure value. */
    value = measure;

    } // end GetValueAndDerivative()

} // end namespace itk

#endif // ITKPCAMETRIC_HXX
