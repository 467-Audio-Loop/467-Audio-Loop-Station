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
#define THIN_LINE 2.0f
#define ROUNDED_CORNER_SIZE 8.0f
#define MAIN_BACKGROUND_COLOR juce::Colours::white
#define MAIN_DRAW_COLOR juce::Colours::black
#define SECONDARY_DRAW_COLOR juce::Colours::grey

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
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
        g.setColour(MAIN_DRAW_COLOR);
        const juce::Rectangle<float> area(button.getLocalBounds().reduced(8).toFloat());
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
        g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), ROUNDED_CORNER_SIZE, THIN_LINE);

        juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
        juce::Path path;
        path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
        path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
        path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);

        g.setColour(box.findColour(juce::ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
        g.strokePath(path, juce::PathStrokeType(2.0f));
    }


    juce::Font titleFont{ 26, 1 };
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
            setOutline(MAIN_DRAW_COLOR, 10);
        }
            break;
        case Play:
        {
            juce::Point<float> point1(0.0f, 0.0f);
            juce::Point<float> point2(0.0f, diameter);
            juce::Point<float> point3(diameter, diameter/2);
            path.addTriangle(point1, point2, point3);
            path = path.createPathWithRoundedCorners(cornerRadius);
            setShape(path, 1, 1, 0);
            setOutline(MAIN_DRAW_COLOR, 10);
        }
            break;
        case Record:
            juce::Rectangle<float> rect(diameter, diameter);
            path.addEllipse(rect);
            setShape(path, 1, 1, 0);
            setOutline(MAIN_DRAW_COLOR, 19);
            break;
        }


    }


private:
    TransportButtonRole buttonRole;
    float cornerRadius = 0.5;
    float diameter = 8.0f;
};