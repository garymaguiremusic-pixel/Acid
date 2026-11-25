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

    // screenshots
    if (argc > 3)
    {
        processor->setPlayHead (nullptr);
        for (int theme = 0; theme < 2; ++theme)
        {
            processor->gui.lightTheme.store (theme == 1);
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
    // one more: the Sequencer page with the Sequence Browser open (browserShot)
    if (argc > 3)
    {
        processor->gui.lightTheme.store (false);
        processor->gui.page.store (0);
        processor->setSequenceBrowserOpen (true);
        std::unique_ptr<AudioProcessorEditor> editor (processor->createEditor());
        editor->setSize (1000, 900);
        auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        PNGImageFormat png;
        auto shot = outDir.getChildFile ("page0_browser.png");
        shot.deleteFile();
        FileOutputStream stream (shot);
        png.writeImageToStream (image, stream);
        processor->setSequenceBrowserOpen (false);
    }
    return peak > 0.001f ? 0 : 1;
}
