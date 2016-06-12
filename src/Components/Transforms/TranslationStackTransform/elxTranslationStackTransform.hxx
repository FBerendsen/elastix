/*=========================================================================
 *
 *  Copyright UMC Utrecht and contributors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __elxTranslationStackTransform_hxx
#define __elxTranslationStackTransform_hxx

#include "elxTranslationStackTransform.h"

#include "itkImageRegionExclusionConstIteratorWithIndex.h"
#include "vnl/vnl_math.h"

namespace elastix
{

/**
* ********************* Constructor ****************************
*/
template <class TElastix>
TranslationStackTransform<TElastix>
::TranslationStackTransform()
{

    xl::xout["error"] << "Constructor" << std::endl;

} // end Constructor


template <class TElastix>
unsigned int TranslationStackTransform<TElastix>
::InitializeTranslationTransform()
{
      xl::xout["error"]<<"InitializeTranslationTransform" << std::endl;
  
    this->m_TranslationDummySubTransform = ReducedDimensionTranslationTransformType::New();
    
    /** Create stack transform. */
    this->m_TranslationStackTransform = TranslationStackTransformType::New();

    /** Set stack transform as current transform. */
    this->SetCurrentTransform( this->m_TranslationStackTransform );

    return 0;
}

/**
 * ******************* BeforeAll ***********************
 */

template <class TElastix>
int TranslationStackTransform<TElastix>
::BeforeAll( void )
{
  xl::xout["error"]<<"BeforeAll" << std::endl;

  /** Initialize translation transform. */
  return InitializeTranslationTransform();
}

/**
 * ******************* BeforeRegistration ***********************
 */

template <class TElastix>
void TranslationStackTransform<TElastix>::BeforeRegistration( void )
{
  xl::xout["error"]<<"BeforeRegistration" << std::endl;

  /** Task 1 - Set the stack transform parameters. */

  /** Determine stack transform settings. Here they are based on the fixed image. */
  const SizeType imageSize = this->GetElastix()->GetFixedImage()->GetLargestPossibleRegion().GetSize();
  this->m_NumberOfSubTransforms = imageSize[ SpaceDimension - 1 ];
  this->m_StackSpacing = this->GetElastix()->GetFixedImage()->GetSpacing()[ SpaceDimension - 1 ];
  this->m_StackOrigin = this->GetElastix()->GetFixedImage()->GetOrigin()[ SpaceDimension - 1 ];

  /** Set stack transform parameters. */
  this->m_TranslationStackTransform->SetNumberOfSubTransforms( this->m_NumberOfSubTransforms );
  this->m_TranslationStackTransform->SetStackOrigin( this->m_StackOrigin );
  this->m_TranslationStackTransform->SetStackSpacing( this->m_StackSpacing );

  /** Initialize stack sub transforms. */
  this->m_TranslationStackTransform->SetAllSubTransforms( this->m_TranslationDummySubTransform );

    this->InitializeTransform();
} // end BeforeRegistration()


/**
 * ********************* InitializeTransform ****************************
 */

template <class TElastix>
void TranslationStackTransform<TElastix>::InitializeTransform()
{
    xl::xout["error"]<<"InitializeTransform" << std::endl;

    /** Check if user wants automatic transform initialization; false by default. */
    bool automaticStackTransformInitialization = false;
    bool tmpBool                          = false;
    this->m_Configuration->ReadParameter( tmpBool,
                                         "AutomaticStackTransformInitialization", 0 );
    if( tmpBool && this->Superclass1::GetInitialTransform() == 0 )
    {
        automaticStackTransformInitialization = true;
    }
    
    if( automaticStackTransformInitialization )
    {
        /** Use the TransformInitializer to determine an initial translation */
        StackTransformInitializerPointer transformInitializer
        = StackTransformInitializerType::New();
        transformInitializer->SetMovingImage( this->m_Registration->GetAsITKBaseType()->GetMovingImage() );
        transformInitializer->SetMovingMask( this->GetElastix()->GetMovingMask() );
        transformInitializer->SetTransform( this->m_TranslationStackTransform );

        transformInitializer->InitializeTransform();
        
//        // DIT STAAT ER EIGNELIJK TE VEEL EN MOET LATER VERWIJDERD WORDEN
//        ParametersType dummyInitialParameters( this->GetNumberOfParameters() );
//        dummyInitialParameters.Fill( 0.0 );

        /** Set the initial parameters in this->m_Registration.*/
        this->m_Registration->GetAsITKBaseType()->
        SetInitialTransformParameters( this->GetParameters() );
        
        /** Give feedback. */
        // \todo: should perhaps also print fixed parameters
        elxout << "Transform parameters are initialized as: "
        << this->GetParameters() << std::endl;

    }
    
    else
    {
        /** Task 2 - Give the registration an initial parameter-array. */
        ParametersType dummyInitialParameters( this->GetNumberOfParameters() );
        dummyInitialParameters.Fill( 0.0 );
        
        /** Put parameters in the registration. */
        this->m_Registration->GetAsITKBaseType()->SetInitialTransformParameters( dummyInitialParameters );
        
        /** Give feedback. */
        // \todo: should perhaps also print fixed parameters
        elxout << "Transform parameters are initialized as: "
        << this->GetParameters() << std::endl;

    }

} // end InitializeTransform()


/**
 * ************************* ReadFromFile ************************
 */

template <class TElastix>
void TranslationStackTransform<TElastix>
::ReadFromFile( void )
{
    xl::xout["error"] << "ReadFromFile" << std::endl;

  /** Read stack-spacing, stack-origin and number of sub-transforms. */
  this->GetConfiguration()->ReadParameter( this->m_NumberOfSubTransforms,
    "NumberOfSubTransforms", this->GetComponentLabel(), 0, 0 );
  this->GetConfiguration()->ReadParameter( this->m_StackOrigin,
    "StackOrigin", this->GetComponentLabel(), 0, 0 );
  this->GetConfiguration()->ReadParameter( this->m_StackSpacing,
    "StackSpacing", this->GetComponentLabel(), 0, 0 );

  /** Initialize translation transform. */
  InitializeTranslationTransform();

  /** Set stack transform parameters. */
  this->m_TranslationStackTransform->SetNumberOfSubTransforms( this->m_NumberOfSubTransforms );
  this->m_TranslationStackTransform->SetStackOrigin( this->m_StackOrigin );
  this->m_TranslationStackTransform->SetStackSpacing( this->m_StackSpacing );

  /** Set stack subtransforms. */
  this->m_TranslationStackTransform->SetAllSubTransforms( this->m_TranslationDummySubTransform );

  /** Call the ReadFromFile from the TransformBase. */
  this->Superclass2::ReadFromFile();

} // end ReadFromFile()


/**
 * ************************* WriteToFile ************************
 *
 * Saves the TransformParameters as a vector and if wanted
 * also as a deformation field.
 */

template <class TElastix>
void TranslationStackTransform<TElastix>
::WriteToFile( const ParametersType & param ) const
{
  xl::xout["error"]<<"WriteToFile" << std::endl;

  /** Call the WriteToFile from the TransformBase. */
  this->Superclass2::WriteToFile( param );

  /** Add some TranslationTransform specific lines. */
  xout["transpar"] << std::endl << "// TranslationStackTransform specific" << std::endl;

     /** Write the stack spacing, stack origin and number of sub transforms. */
  xout["transpar"] << "(StackSpacing " << this->m_TranslationStackTransform->GetStackSpacing() << ")" << std::endl;
  xout["transpar"] << "(StackOrigin " << this->m_TranslationStackTransform->GetStackOrigin() << ")" << std::endl;
  xout["transpar"] << "(NumberOfSubTransforms " << this->m_TranslationStackTransform->GetNumberOfSubTransforms() << ")" << std::endl;

  /** Set the precision back to default value. */
  xout["transpar"] << std::setprecision(
    this->m_Elastix->GetDefaultOutputPrecision() );

} // end WriteToFile()

} // end namespace elastix


#endif // end #ifndef __elxTranslationStackTransform_hxx
