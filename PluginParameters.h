#pragma once

#include <JuceHeader.h>

#define MAX_NUM_BANDS 10

//creiamo un namespace
namespace Parameters {

	static const String globalGain = "globalGain";

	static const float defaultGain = 0.0f;

	static const float dbFloor = 12.0f;

	static const String useSidechain = "useSidechain";

	//funzioni helper (per parsing) --> da usare negli altri file anche per avere parsing corretto 
	static juce::String getBandParamID(int bandIndex) {
		return "band_" + juce::String(bandIndex);
	}

	static juce::String getBandDisplayName(int bandIndex) {
		return "Band " + juce::String(bandIndex + 1);
	}

	static AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
		std::vector<std::unique_ptr<RangedAudioParameter>> params;

		int id = 1;

		for (int band = 0; band < MAX_NUM_BANDS; ++band) //creo tanti parametri quanti MAX_NUM_BANDS
		{
			params.push_back(std::make_unique<juce::AudioParameterFloat>(
				juce::ParameterID(getBandParamID(band), id++),
				getBandDisplayName(band),
				juce::NormalisableRange<float>(-dbFloor, +dbFloor, 0.5f),
				defaultGain
			));
		}

		params.push_back(std::make_unique<AudioParameterFloat>(ParameterID(globalGain, id++), "Output Gain", NormalisableRange<float>(-24.0f, +24.0f, 0.1f), 0.0f));
		params.push_back(std::make_unique<AudioParameterBool>(ParameterID(useSidechain, id++), "Use Sidechain as Reference", false));

		return { params.begin(), params.end() };
	}

	static void addListenerToAllParameters(AudioProcessorValueTreeState& valueTreeState, AudioProcessorValueTreeState::Listener* listener) {
		std::unique_ptr<XmlElement> xml(valueTreeState.copyState().createXml());
		for (auto* element : xml->getChildWithTagNameIterator("PARAM")) {
			const String& id = element->getStringAttribute("id");
			valueTreeState.addParameterListener(id, listener);

		}
	}
}
