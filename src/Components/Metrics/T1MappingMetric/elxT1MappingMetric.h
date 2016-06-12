/*======================================================================

  This file is part of the elastix software.

  Copyright (c) University Medical Center Utrecht. All rights reserved.
  See src/CopyrightElastix.txt or http://elastix.isi.uu.nl/legal.php for
  details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE. See the above copyright notices for more information.

======================================================================*/
#ifndef __elxT1MappingMetric_H__
#define __elxT1MappingMetric_H__

#include "elxIncludes.h"
#include "itkT1MappingMetric.h"
#include "itkAdvancedBSplineDeformableTransform.h"
#include "itkStackTransform.h"


namespace elastix
{

  /** \class T1MappingMetric
   * \brief
   *
   * \ingroup RegistrationMetrics
   * \ingroup Metrics
   */

  template <class TElastix >
    class T1MappingMetric:
    public
      itk::T1MappingMetric<
        typename MetricBase<TElastix>::FixedImageType,
        typename MetricBase<TElastix>::MovingImageType >,
    public MetricBase<TElastix>
  {
  public:

    /** Standard ITK-stuff. */
    typedef T1MappingMetric              Self;
    typedef itk::T1MappingMetric<
      typename MetricBase<TElastix>::FixedImageType,
      typename MetricBase<TElastix>::MovingImageType >    Superclass1;
    typedef MetricBase<TElastix>                          Superclass2;
    typedef itk::SmartPointer<Self>                       Pointer;
    typedef itk::SmartPointer<const Self>                 ConstPointer;

    /** Method for creation through the object factory. */
    itkNewMacro( Self );

    /** Run-time type information (and related methods). */
    itkTypeMacro( T1MappingMetric, itk::T1MappingMetric );

    /** Name of this class.
     * Use this name in the parameter file to select this specific metric. \n
     * example: <tt>(Metric "VarianceOverLastDimensionMetric")</tt>\n
     */
    elxClassNameMacro( "T1MappingMetric" );

    /** Typedefs from the superclass. */
    typedef typename
      Superclass1::CoordinateRepresentationType              CoordinateRepresentationType;
    typedef typename Superclass1::ScalarType                 ScalarType;
    typedef typename Superclass1::MovingImageType            MovingImageType;
    typedef typename Superclass1::MovingImagePixelType       MovingImagePixelType;
    typedef typename Superclass1::MovingImageConstPointer    MovingImageConstPointer;
    typedef typename Superclass1::FixedImageType             FixedImageType;
    typedef typename Superclass1::FixedImageConstPointer     FixedImageConstPointer;
    typedef typename Superclass1::FixedImageRegionType       FixedImageRegionType;
    typedef typename Superclass1::FixedImageSizeType         FixedImageSizeType;
    typedef typename Superclass1::TransformType              TransformType;
    typedef typename Superclass1::TransformPointer           TransformPointer;
    typedef typename Superclass1::InputPointType             InputPointType;
    typedef typename Superclass1::OutputPointType            OutputPointType;
    typedef typename Superclass1::TransformParametersType    TransformParametersType;
    typedef typename Superclass1::TransformJacobianType      TransformJacobianType;
    typedef typename Superclass1::InterpolatorType           InterpolatorType;
    typedef typename Superclass1::InterpolatorPointer        InterpolatorPointer;
    typedef typename Superclass1::RealType                   RealType;
    typedef typename Superclass1::GradientPixelType          GradientPixelType;
    typedef typename Superclass1::GradientImageType          GradientImageType;
    typedef typename Superclass1::GradientImagePointer       GradientImagePointer;
    typedef typename Superclass1::GradientImageFilterType    GradientImageFilterType;
    typedef typename Superclass1::GradientImageFilterPointer GradientImageFilterPointer;
    typedef typename Superclass1::FixedImageMaskType         FixedImageMaskType;
    typedef typename Superclass1::FixedImageMaskPointer      FixedImageMaskPointer;
    typedef typename Superclass1::MovingImageMaskType        MovingImageMaskType;
    typedef typename Superclass1::MovingImageMaskPointer     MovingImageMaskPointer;
    typedef typename Superclass1::MeasureType                MeasureType;
    typedef typename Superclass1::DerivativeType             DerivativeType;
    typedef typename Superclass1::ParametersType             ParametersType;
    typedef typename Superclass1::FixedImagePixelType        FixedImagePixelType;
    typedef typename Superclass1::MovingImageRegionType      MovingImageRegionType;
    typedef typename Superclass1::ImageSamplerType           ImageSamplerType;
    typedef typename Superclass1::ImageSamplerPointer        ImageSamplerPointer;
    typedef typename Superclass1::ImageSampleContainerType   ImageSampleContainerType;
    typedef typename
      Superclass1::ImageSampleContainerPointer               ImageSampleContainerPointer;
    typedef typename Superclass1::FixedImageLimiterType      FixedImageLimiterType;
    typedef typename Superclass1::MovingImageLimiterType     MovingImageLimiterType;
    typedef typename
      Superclass1::FixedImageLimiterOutputType               FixedImageLimiterOutputType;
    typedef typename
      Superclass1::MovingImageLimiterOutputType              MovingImageLimiterOutputType;
    typedef typename
      Superclass1::MovingImageDerivativeScalesType           MovingImageDerivativeScalesType;

    /** The fixed image dimension. */
    itkStaticConstMacro( FixedImageDimension, unsigned int,
      FixedImageType::ImageDimension );

    /** The moving image dimension. */
    itkStaticConstMacro( MovingImageDimension, unsigned int,
      MovingImageType::ImageDimension );

    /** Typedef's inherited from Elastix. */
    typedef typename Superclass2::ElastixType               ElastixType;
    typedef typename Superclass2::ElastixPointer            ElastixPointer;
    typedef typename Superclass2::ConfigurationType         ConfigurationType;
    typedef typename Superclass2::ConfigurationPointer      ConfigurationPointer;
    typedef typename Superclass2::RegistrationType          RegistrationType;
    typedef typename Superclass2::RegistrationPointer       RegistrationPointer;
    typedef typename Superclass2::ITKBaseType               ITKBaseType;

    /** Typedef's for the B-spline transform. */
    typedef itk::AdvancedBSplineDeformableTransformBase<
      ScalarType, FixedImageDimension >                       BSplineTransformBaseType;
    typedef itk::AdvancedCombinationTransform<
      ScalarType, FixedImageDimension >                       CombinationTransformType;
    typedef itk::StackTransform<
      ScalarType, FixedImageDimension, MovingImageDimension > StackTransformType;
    typedef itk::AdvancedBSplineDeformableTransformBase<
      ScalarType, FixedImageDimension - 1 >                   ReducedDimensionBSplineTransformBaseType;

    /** Sets up a timer to measure the initialization time and
     * calls the Superclass' implementation.
     */
    virtual void Initialize(void) throw (itk::ExceptionObject);

    /**
     * Do some things before each resolution:
     * \li Set CheckNumberOfSamples setting
     * \li Set UseNormalization setting
     */
    virtual void BeforeEachResolution(void);
    virtual void AfterEachIteration(void);

  protected:

    /** The constructor. */
    T1MappingMetric(){};
    /** The destructor. */
    virtual ~T1MappingMetric() {}

  private:

    /** The private constructor. */
    T1MappingMetric( const Self& ); // purposely not implemented

    /** The private copy constructor. */
    void operator=( const Self& );              // purposely not implemented

  }; // end class T1MappingMetric


} // end namespace elastix


#ifndef ITK_MANUAL_INSTANTIATION
#include "elxT1MappingMetric.hxx"
#endif

#endif // end #ifndef __elxVarianceOverLastDimensionMetric_H__
