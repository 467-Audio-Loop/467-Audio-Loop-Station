/*
  ==============================================================================

    customUI.h
    Created: 26 Feb 2021 9:46:53pm
    Author:  dnego

    header file containing custom classes for our loopstation UI
  ==============================================================================
*/

#pragma once

#define THICK_LINE 5.0f
#define THIN_LINE 3.0f
#define PLAY_STOP_LINE_THICKNESS 8
#define ROUNDED_CORNER_SIZE 8.0f
#define MAIN_BACKGROUND_COLOR juce::Colours::white
#define MAIN_DRAW_COLOR juce::Colours::black
#define SECONDARY_DRAW_COLOR juce::Colours::grey
#define GAP_FOR_OUTLINE 1.2f
#define METRONOME_ON_COLOR juce::Colours::limegreen
#define EDITOR_FONT juce::Font(14.0f,juce::Font::bold)
#define LABEL_FONT juce::Font(14.0f,juce::Font::bold)

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {

        setColour(juce::TextEditor::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::TextEditor::outlineColourId, MAIN_DRAW_COLOR);
        setColour(juce::TextEditor::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::TextEditor::highlightedTextColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::TextEditor::highlightColourId, MAIN_DRAW_COLOR);
        setColour(juce::TextEditor::focusedOutlineColourId, MAIN_DRAW_COLOR);
        setColour(juce::AlertWindow::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::AlertWindow::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::TextButton::buttonColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::TextButton::textColourOffId, MAIN_DRAW_COLOR);
        setColour(juce::TextButton::textColourOnId, MAIN_DRAW_COLOR);
        setColour(juce::ComboBox::outlineColourId, MAIN_DRAW_COLOR);
        setColour(juce::ComboBox::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::ComboBox::outlineColourId, MAIN_DRAW_COLOR);
        setColour(juce::ComboBox::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::PopupMenu::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::PopupMenu::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::Label::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::Label::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::ListBox::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::ListBox::textColourId, MAIN_DRAW_COLOR);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/,
        bool /*isMouseOverButton*/, bool /*isButtonDown*/)
    {
        const juce::Rectangle<float> area(button.getLocalBounds().reduced(8).toFloat());
        g.setColour(button.findColour(juce::TextButton::buttonColourId));
        g.fillRoundedRectangle(area, ROUNDED_CORNER_SIZE);
            
        g.setColour(MAIN_DRAW_COLOR);

        g.drawRoundedRectangle(area, ROUNDED_CORNER_SIZE, THIN_LINE);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
        int, int, int, int, juce::ComboBox& box)
    {
        //auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        juce::Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), ROUNDED_CORNER_SIZE);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(GAP_FOR_OUTLINE, GAP_FOR_OUTLINE), ROUNDED_CORNER_SIZE, THIN_LINE);

        juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
        juce::Path path;
        path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
    {
        if (dynamic_cast<juce::AlertWindow*> (textEditor.getParentComponent()) == nullptr)
        {
            if (textEditor.isEnabled())
            {
                setColour(juce::TextEditor::textColourId, MAIN_DRAW_COLOR);
                textEditor.setText(textEditor.getText()); //DN:refreshes the color
                
                if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly())
                {
                    g.setColour(textEditor.findColour(juce::TextEditor::focusedOutlineColourId));
                    //g.drawRect(0, 0, width, height, 2);
                    juce::Rectangle<int> boxBounds(0, 0, width, height);
                    g.drawRoundedRectangle(boxBounds.toFloat().reduced(GAP_FOR_OUTLINE), ROUNDED_CORNER_SIZE, THIN_LINE);
                }
                else
                {
                    g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
                    juce::Rectangle<int> boxBounds(0, 0, width, height);
                    g.drawRoundedRectangle(boxBounds.toFloat().reduced(GAP_FOR_OUTLINE), ROUNDED_CORNER_SIZE, THIN_LINE);
                }
            }
            else
            {
                setColour(juce::TextEditor::textColourId, SECONDARY_DRAW_COLOR);
                textEditor.setText(textEditor.getText()); //DN:refreshes the color
                g.setColour(SECONDARY_DRAW_COLOR);
                juce::Rectangle<int> boxBounds(0, 0, width, height);
                g.drawRoundedRectangle(boxBounds.toFloat().reduced(GAP_FOR_OUTLINE), ROUNDED_CORNER_SIZE, THIN_LINE);
            }
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos,
        float minSliderPos,
        float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        if (slider.isBar())
        {
            auto color = slider.findColour(juce::Slider::trackColourId);
            g.setColour(color);
            g.fillRect(slider.isHorizontal() ? juce::Rectangle<float>(static_cast<float> (x), (float)y + 0.5f, sliderPos - (float)x, (float)height - 1.0f)
                : juce::Rectangle<float>((float)x + 0.5f, sliderPos, (float)width - 1.0f, (float)y + ((float)height - sliderPos)));
        }
        else
        {
            auto isTwoVal = (style == juce::Slider::SliderStyle::TwoValueVertical || style == juce::Slider::SliderStyle::TwoValueHorizontal);
            auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);

            auto trackWidth = juce::jmin(6.0f, slider.isHorizontal() ? (float)height * 0.25f : (float)width * 0.25f);

            juce::Point<float> startPoint(slider.isHorizontal() ? (float)x : (float)x + (float)width * 0.5f,
                slider.isHorizontal() ? (float)y + (float)height * 0.5f : (float)(height + y));

            juce::Point<float> endPoint(slider.isHorizontal() ? (float)(width + x) : startPoint.x,
                slider.isHorizontal() ? startPoint.y : (float)y);

            juce::Path backgroundTrack;
            backgroundTrack.startNewSubPath(startPoint);
            backgroundTrack.lineTo(endPoint);
            g.setColour(slider.findColour(juce::Slider::backgroundColourId));
            g.strokePath(backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

            juce::Path valueTrack;
            juce::Point<float> minPoint, maxPoint, thumbPoint;

            if (isTwoVal || isThreeVal)
            {
                minPoint = { slider.isHorizontal() ? minSliderPos : (float)width * 0.5f,
                             slider.isHorizontal() ? (float)height * 0.5f : minSliderPos };

                if (isThreeVal)
                    thumbPoint = { slider.isHorizontal() ? sliderPos : (float)width * 0.5f,
                                   slider.isHorizontal() ? (float)height * 0.5f : sliderPos };

                maxPoint = { slider.isHorizontal() ? maxSliderPos : (float)width * 0.5f,
                             slider.isHorizontal() ? (float)height * 0.5f : maxSliderPos };
            }
            else
            {
                auto kx = slider.isHorizontal() ? sliderPos : ((float)x + (float)width * 0.5f);
                auto ky = slider.isHorizontal() ? ((float)y + (float)height * 0.5f) : sliderPos;

                minPoint = startPoint;
                maxPoint = { kx, ky };
            }

            auto thumbWidth = getSliderThumbRadius(slider);

            valueTrack.startNewSubPath(minPoint);
            valueTrack.lineTo(isThreeVal ? thumbPoint : maxPoint);
            g.setColour(slider.findColour(juce::Slider::trackColourId));
            g.strokePath(valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });

            if (!isTwoVal)
            {
                g.setColour(MAIN_DRAW_COLOR);
                //g.fillEllipse(juce::Rectangle<float>(static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre(isThreeVal ? thumbPoint : maxPoint));
                
                auto knobArea = juce::Rectangle<float>(static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre(isThreeVal ? thumbPoint : maxPoint);
                //g.fillRoundedRectangle(knobArea.getTopLeft().getX(),knobArea.getTopLeft.getY(),knobArea.getWidth(),knobArea.getHeight());
               // if (slider.isHorizontal())
                    knobArea.expand(2, 2);
                //else
                    //knobArea.expand(8, 4);
                g.fillRect(knobArea);
                g.setColour(MAIN_BACKGROUND_COLOR);
                g.fillRect(knobArea.reduced(4));
            }

            if (isTwoVal || isThreeVal)
            {
                auto sr = juce::jmin(trackWidth, (slider.isHorizontal() ? (float)height : (float)width) * 0.4f);
                auto pointerColour = slider.findColour(juce::Slider::thumbColourId);

                if (slider.isHorizontal())
                {
                    drawPointer(g, minSliderPos - sr,
                        juce::jmax(0.0f, (float)y + (float)height * 0.5f - trackWidth * 2.0f),
                        trackWidth * 2.0f, pointerColour, 2);

                    drawPointer(g, maxSliderPos - trackWidth,
                        juce::jmin((float)(y + height) - trackWidth * 2.0f, (float)y + (float)height * 0.5f),
                        trackWidth * 2.0f, pointerColour, 4);
                }
                else
                {
                    drawPointer(g, juce::jmax(0.0f, (float)x + (float)width * 0.5f - trackWidth * 2.0f),
                        minSliderPos - trackWidth,
                        trackWidth * 2.0f, pointerColour, 1);

                    drawPointer(g, juce::jmin((float)(x + width) - trackWidth * 2.0f, (float)x + (float)width * 0.5f), maxSliderPos - sr,
                        trackWidth * 2.0f, pointerColour, 3);
                }
            }
        }
    }


    juce::Font titleFont{ 26, 1 };
private:

 
};

class SettingsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SettingsLookAndFeel()
    {
        setColour(juce::TextEditor::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::TextEditor::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::AlertWindow::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::AlertWindow::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::TextButton::buttonColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::TextButton::textColourOffId, MAIN_DRAW_COLOR);
        setColour(juce::TextButton::textColourOnId, MAIN_DRAW_COLOR);
        setColour(juce::ComboBox::outlineColourId, MAIN_DRAW_COLOR);
        setColour(juce::ComboBox::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::ComboBox::outlineColourId, MAIN_DRAW_COLOR);
        setColour(juce::ComboBox::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::PopupMenu::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::PopupMenu::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::Label::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::Label::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::ListBox::backgroundColourId, MAIN_BACKGROUND_COLOR);
        setColour(juce::ListBox::textColourId, MAIN_DRAW_COLOR);
        setColour(juce::ListBox::outlineColourId, MAIN_DRAW_COLOR);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/,
        bool /*isMouseOverButton*/, bool /*isButtonDown*/)
    {
        g.setColour(MAIN_DRAW_COLOR);
        const juce::Rectangle<float> area(button.getLocalBounds().reduced(1).toFloat());
        g.drawRoundedRectangle(area, ROUNDED_CORNER_SIZE, THIN_LINE);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool,
        int, int, int, int, juce::ComboBox& box)
    {
        //auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        juce::Rectangle<int> boxBounds(0, 0, width, height);

        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(boxBounds.toFloat(), ROUNDED_CORNER_SIZE);

        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(GAP_FOR_OUTLINE), ROUNDED_CORNER_SIZE, THIN_LINE);

        juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
        juce::Path path;
        path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }


    void drawTickBox(juce::Graphics& g, juce::Component& component,
        float x, float y, float w, float h,
        const bool ticked,
        const bool isEnabled,
        const bool shouldDrawButtonAsHighlighted,
        const bool shouldDrawButtonAsDown)
    {
        juce::ignoreUnused(isEnabled, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

        juce::Rectangle<float> tickBounds(x, y, w, h);

        //g.setColour(component.findColour(juce::ToggleButton::tickDisabledColourId));
        g.setColour(SECONDARY_DRAW_COLOR);
        g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);

        if (ticked)
        {
            //g.setColour(component.findColour(juce::ToggleButton::tickColourId));
            g.setColour(MAIN_DRAW_COLOR);
            auto tick = getTickShape(0.75f);
            g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(4, 5).toFloat(), false));
        }
    }


private:


};


class TransportButton : public juce::ShapeButton
{
public:
    // AF: enum for choosing stop play or record
    enum TransportButtonRole
    {
        Stop,
        Play,
        Record
    };

    TransportButton(const juce::String& t, juce::Colour n, juce::Colour o, juce::Colour d, TransportButtonRole role)
        : juce::ShapeButton(t,n,o,d)
    {
        buttonRole = role;
        juce::Path path;
        switch (buttonRole)
        {
        case Stop:
        {
            juce::Rectangle<float> stopRect(diameter, diameter);
            path.addRoundedRectangle(stopRect, cornerRadius);
            setShape(path, 1, 1, 0);
            setOutline(MAIN_DRAW_COLOR, PLAY_STOP_LINE_THICKNESS);
        }
            break;
        case Play:
        {
            juce::Point<float> point1(0.0f, 0.0f);
            juce::Point<float> point2(0.0f, diameter);
            juce::Point<float> point3(diameter*0.93f, diameter/2);
            path.addTriangle(point1, point2, point3);
            path = path.createPathWithRoundedCorners(cornerRadius);
            setShape(path, 1, 1, 0);
            setOutline(MAIN_DRAW_COLOR, PLAY_STOP_LINE_THICKNESS);
        }
            break;
        case Record:
            juce::Rectangle<float> rect(diameter, diameter);
            path.addEllipse(rect);
            setShape(path, 1, 1, 0);
            setOutline(MAIN_DRAW_COLOR, 15);
            break;
        }


    }

private:
    TransportButtonRole buttonRole;
    float cornerRadius = 0.5;
    float diameter = 8.0f;
};



class LoopLengthButton : public juce::DrawableButton
{
public:
    // AF: enum for choosing stop play or record


    LoopLengthButton(const juce::String& buttonName,
        ButtonStyle buttonStyle) : juce::DrawableButton(buttonName,buttonStyle)
    {

    }

    void mouseEnter(const juce::MouseEvent& event)
    {
        if (isEnabled())
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }


    void mouseExit(const juce::MouseEvent& event)
    {
        if (isEnabled())
            setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseDown(const juce::MouseEvent& event)
    {
        oldBeats = beatsBox->getText().getIntValue();
    }

    void mouseDrag(const juce::MouseEvent& event)
    {
        if (isEnabled())
        {
            int distance = event.getDistanceFromDragStartX();
            DBG("distance = " + juce::String(distance));
            int difference = distance / 35;
            newBeats = oldBeats + difference;
            if (newBeats < 1)
                newBeats = 1;
            if (newBeats > 32)
                newBeats = 32;

            DBG("new beats = " + juce::String(newBeats));
            beatsBox->setText(juce::String(newBeats), true);
        }

    }

    void mouseUp(const juce::MouseEvent& event)
    {

    }

    void setBeatsBox(juce::TextEditor* newBeatsBox)
    {
        beatsBox = newBeatsBox;
    }


private:
    int oldBeats = 8;
    int newBeats = 8;
    juce::TextEditor* beatsBox;

    bool shouldDrawButtonAsDown = false;
    juce::BorderSize<int> border;
};