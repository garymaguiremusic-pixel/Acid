// Offline check: plays the plugin against a simulated host transport, writes a WAV,
// and saves screenshots of every page (run under a virtual display on Linux).
//   RenderCheck <outputFolder> [presetIndex]
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../src/PluginProcessor.h"
#include "../src/PluginEditor.h"

using namespace juce;

struct FakePlayHead : AudioPlayHead
{
    double ppq = 0.0, bpm = 128.0;
    bool playing = true;
    Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm (bpm);
        info.setPpqPosition (ppq);
        info.setIsPlaying (playing);
        info.setTimeSignature (TimeSignature { 4, 4 });
        return info;
    }
};

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const File outDir (argc > 1 ? File::getCurrentWorkingDirectory().getChildFile (argv[1]) : File::getCurrentWorkingDirectory());
    outDir.createDirectory();
    const int preset = argc > 2 ? String (argv[2]).getIntValue() : 0;

    auto processor = std::make_unique<AcidAudioProcessor>();
    processor->setCurrentProgram (preset);
    const double sr = 48000.0;
    const int block = 480;
    processor->setPlayConfigDetails (0, 2, sr, block);
    processor->prepareToPlay (sr, block);

    FakePlayHead head;
    processor->setPlayHead (&head);

    const int seconds = 8;
    AudioBuffer<float> out (2, (int) sr * seconds);
    AudioBuffer<float> buf (2, block);
    MidiBuffer midi;
    float peak = 0.0f;
    int midiOutEvents = 0;
    for (int pos = 0; pos + block <= out.getNumSamples(); pos += block)
    {
        midi.clear();
        processor->processBlock (buf, midi);
        midiOutEvents += midi.getNumEvents();
        for (int c = 0; c < 2; ++c)
            out.copyFrom (c, pos, buf, c, 0, block);
        peak = jmax (peak, buf.getMagnitude (0, block));
        head.ppq += block / sr * head.bpm / 60.0;
    }

    WavAudioFormat wav;
    auto file = outDir.getChildFile ("render_preset" + String (preset) + ".wav");
    file.deleteFile();
    if (auto writer = std::unique_ptr<AudioFormatWriter> (wav.createWriterFor (new FileOutputStream (file), sr, 2, 24, {}, 0)))
        writer->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());

    std::cout << "preset " << preset << " '" << processor->getProgramName (preset) << "'  peak " << peak
              << "  midi out events " << midiOutEvents << "  latency " << processor->getLatencySamples() << std::endl;

    // the same pattern on the 303: dry, through Saturate / Foldback at three drives, and with Layers
    {
        struct Case { const char* name; float distOn, mode, drive, oscB, unison; };
        const Case cases[] = { { "303_dry", 0, 0, 0, 0, 1 },
                               { "303_sat_drive0", 1, 0, 0, 0, 1 }, { "303_sat_drive50", 1, 0, 50, 0, 1 }, { "303_sat_drive100", 1, 0, 100, 0, 1 },
                               { "303_fold_drive0", 1, 1, 0, 0, 1 }, { "303_fold_drive50", 1, 1, 50, 0, 1 }, { "303_fold_drive100", 1, 1, 100, 0, 1 },
                               { "303_layers", 0, 0, 0, 60, 4 } };
        processor->setParameterValue ("engine", 1.0f);
        for (const auto& k : cases)
        {
            processor->setParameterValue ("distOn", k.distOn);
            processor->setParameterValue ("distMode", k.mode);
            processor->setParameterValue ("distDrive", k.drive);
            processor->setParameterValue ("distAmount", 100.0f);
            processor->setParameterValue ("oscBLevel", k.oscB);
            processor->setParameterValue ("unisonVoices", k.unison);
            processor->reset();
            head.ppq = 0.0;
            AudioBuffer<float> o (2, (int) sr * 4);
            for (int pos = 0; pos + block <= o.getNumSamples(); pos += block)
            {
                midi.clear();
                processor->processBlock (buf, midi);
                for (int c = 0; c < 2; ++c)
                    o.copyFrom (c, pos, buf, c, 0, block);
                head.ppq += block / sr * head.bpm / 60.0;
            }
            auto f = outDir.getChildFile (String (k.name) + ".wav");
            f.deleteFile();
            if (auto writer = std::unique_ptr<AudioFormatWriter> (wav.createWriterFor (new FileOutputStream (f), sr, 2, 24, {}, 0)))
                writer->writeFromAudioSampleBuffer (o, 0, o.getNumSamples());
            const int skip = (int) sr / 2;   // past the parameter smoothing
            std::cout << k.name << "  peak " << o.getMagnitude (skip, o.getNumSamples() - skip)
                      << "  rms dB " << Decibels::gainToDecibels (o.getRMSLevel (0, skip, o.getNumSamples() - skip)) << std::endl;
        }
        processor->setParameterValue ("distOn", 0.0f);
        processor->setParameterValue ("oscBLevel", 0.0f);
        processor->setParameterValue ("unisonVoices", 1.0f);
    }

    // screenshots
    if (argc > 3)
    {
        processor->setPlayHead (nullptr);
        for (int theme = 0; theme < 2; ++theme)
        {
            processor->gui.lightTheme.store (theme == 1);
            processor->setParameterValue ("engine", 0.0f);
            for (int page = 0; page < 3; ++page)
            {
                processor->gui.page.store (page);
                std::unique_ptr<AudioProcessorEditor> editor (processor->createEditor());
                editor->setSize (1000, 900);
                for (int t = 0; t < 5; ++t) { editor->getChildComponent (0)->resized(); }
                auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
                PNGImageFormat png;
                auto shot = outDir.getChildFile ("page" + String (page) + (theme ? "_light" : "_dark") + ".png");
                shot.deleteFile();
                FileOutputStream stream (shot);
                png.writeImageToStream (image, stream);
            }
        }
    }
    if (argc > 3)
    {
        // the Synth / Seq page with the 303 selected
        processor->setPlayHead (nullptr);
        processor->gui.lightTheme.store (false);
        processor->gui.page.store (0);
        processor->setParameterValue ("engine", 1.0f);
        std::unique_ptr<AudioProcessorEditor> editor (processor->createEditor());
        editor->setSize (1000, 900);
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        auto shot = outDir.getChildFile ("page0_303.png");
        shot.deleteFile();
        FileOutputStream stream (shot);
        PNGImageFormat().writeImageToStream (image, stream);
    }
    return peak > 0.001f ? 0 : 1;
}
