/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2010                            */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alok Parlikar (aup@cs.cmu.edu)                   */
/*               Date:  March 2010                                       */
/*************************************************************************/
/*                                                                       */
/*  Library classes to manage available flite voices                     */
/*                                                                       */
/*************************************************************************/

#include "edu_cmu_cs_speech_tts_fliteVoices.hh"
#include "edu_cmu_cs_speech_tts_Common.hh"

namespace FliteEngine {
  
  Voice::Voice(const String flang, const String fcountry, const String fvar, 
	       t_voice_register_function freg, 
	       t_voice_unregister_function funreg)
  {
    language = flang;
    country = fcountry;
    variant = fvar;
    fliteVoice = NULL;
    regfunc = freg;
    unregfunc = funreg;
  }
  
  Voice::~Voice()
  {
    LOGI("Voice::~Voice: unregistering voice");
    unregisterVoice();
    LOGI("Voice::~Voice: voice unregistered");
  }


  const char* Voice::getLanguage() 
  {
    return language.c_str();
  }

  const char* Voice::getCountry() 
  {
    return country.c_str();
  }
  
  const char* Voice:: getVariant() 
  {
    return variant.c_str();
  }

  bool Voice::isSameLocaleAs(String flang, String fcountry, String fvar)
  {
    LOGI("Voice::isSameLocaleAs");
    if( (language == flang) &&
	(country == fcountry) &&
	(variant == fvar) )
      return true;
    else
      return false;
  }

  cst_voice* Voice::registerVoice()
  {
    LOGI("Voice::registerVoice for %s",variant.c_str());
    fliteVoice = regfunc(voxdir_path);
    LOGI("Voice::registerVoice done");
    return fliteVoice;
  }

  void Voice::unregisterVoice()
  {
    LOGI("Calling flite unregister for %s",variant.c_str());
    if(fliteVoice == NULL) return; // Voice not registered
    unregfunc(fliteVoice);
    LOGI("Done unregistering voice in flite");
    fliteVoice = NULL;
  }
  
  cst_voice* Voice::getFliteVoice()
  {
    return fliteVoice;
  }

  Voices::Voices(int fmaxCount, VoiceRegistrationMode fregistrationMode)
  {
    
    rMode = fregistrationMode;
    currentVoice = NULL;
    voiceList = new Voice*[fmaxCount];
    maxCount = fmaxCount;
    currentCount = 0;
  }

  Voices::~Voices()
  {
    LOGI("Voices::~Voices Deleting voice list");
    if(voiceList != NULL)
      {
	for(int i=0;i<currentCount;i++)
	  if(voiceList[i] != NULL)
	    delete voiceList[i]; // Delete the individual voices
	delete[] voiceList;
	voiceList = NULL;
      }
    LOGI("Voices::~Voices voice list deleted");
  }

  Voice* Voices::getCurrentVoice()
  {
    return currentVoice;
  }

  void Voices::addVoice(String flang, String fcountry, String fvar, 
			t_voice_register_function freg,
			t_voice_unregister_function funreg)
  {
    LOGI("Voices::addVoice adding %s",fvar.c_str());
    if(currentCount==maxCount)
      {
	LOGE("Could not add voice %s_%s_%s. Too many voices",
	     flang.c_str(),fcountry.c_str(), fvar.c_str());
	return;
      }
    
    Voice* v = new Voice(flang, fcountry, fvar, freg, funreg);

    /* We must register this voice if the registration mode
       so dictates.
    */

    if(rMode == ALL_VOICES_REGISTERED)
        v->registerVoice();

    voiceList[currentCount] = v;
    currentCount++;

    // Set a default voice.
    if(currentVoice == NULL)
      currentVoice = getVoiceForLocale(flang, fcountry, fvar); // Take care of voice registration issues
  }

  bool Voices::isLocaleAvailable(String flang, String fcountry, String fvar)
  {
    LOGI("Voices::isLocaleAvailable");
    for(int i=0; i<currentCount;i++)
      {
	if(voiceList[i] == NULL) continue;
	if(voiceList[i]->isSameLocaleAs(flang, fcountry, fvar))
	  {
	    return true;
	  }
      }
    return false;
  }

  Voice* Voices::getVoiceForLocale(String flang, 
				   String fcountry, String fvar)
  {
    LOGI("Voices::getVoiceForLocale");
    Voice* ptr;
    for(int i=0; i<currentCount;i++)
      {
	ptr = voiceList[i];
	if(ptr->isSameLocaleAs(flang, fcountry, fvar))
	  {
	    if(rMode == ALL_VOICES_REGISTERED)
	      {
		currentVoice = ptr;
		return currentVoice;
	      }
	    else
	      {
		    /* Only one voice can be registered.
		   Check that the one that the user wants
		   isn't already the current voice.
		   Otherwise, unregister current one
		   and then register and set the requested one
		*/
		
		if(ptr == currentVoice)
		  {
		    LOGD("Requested voice is the current voice!");
		    return currentVoice;
		  }
		else
		  {
		    LOGD("Requested voice is not registered. Need to register");
		    if(currentVoice!= NULL)
		      currentVoice->unregisterVoice();
		    currentVoice = ptr;
		    currentVoice->registerVoice();
		    return currentVoice;
		  }
		
	      }
	  }
      }
    currentVoice = NULL; // Requested voice not available!
    return currentVoice;
  }
}
