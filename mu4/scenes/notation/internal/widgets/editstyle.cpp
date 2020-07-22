//=============================================================================
//  MuseScore
//  Music Composition & Notation
//
//  Copyright (C) 2002-2016 Werner Schweer and others
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================
#include "editstyle.h"

#include <QVariantList>

#include "libmscore/score.h"
#include "scoreview.h"
#include "libmscore/style.h"
#include "libmscore/articulation.h"
#include "libmscore/sym.h"
#include "icons.h"
//#include "musescore.h"
#include "libmscore/undo.h"
#include "icons.h"
#include "libmscore/harmony.h"
#include "libmscore/chordlist.h"
#include "libmscore/figuredbass.h"
#include "libmscore/clef.h"
#include "libmscore/excerpt.h"
#include "libmscore/tuplet.h"
#include "libmscore/layout.h"
#include "alignSelect.h"
#include "offsetSelect.h"
#include "fontStyleSelect.h"
//#include "preferences.h"

#include "ui/view/iconcodes.h"

using namespace mu::scene::notation;
using namespace mu::domain::notation;

//---------------------------------------------------------
//   EditStyle
//---------------------------------------------------------

EditStyle::EditStyle(QWidget* parent)
    : QDialog(parent)
{
    setObjectName("EditStyle");
    setupUi(this);
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    buttonApplyToAllParts = buttonBox->addButton(tr("Apply to all Parts"), QDialogButtonBox::ApplyRole);
    //buttonApplyToAllParts->setEnabled(!cs->isMaster()); // set in showEvent() now
//    buttonTogglePagelist->setIcon(QIcon(IconCode::Code::ACCOUNT));
    setModal(true);

    // create button groups for every set of radio button widgets
    // use this group widgets in list styleWidgets
    // This works for groups which represent an int enumeration.

    QButtonGroup* fretNumGroup = new QButtonGroup(this);
    fretNumGroup->addButton(radioFretNumLeft, 0);
    fretNumGroup->addButton(radioFretNumRight, 1);

    QButtonGroup* ksng = new QButtonGroup(this);
    ksng->addButton(radioKeySigNatNone, int(KeySigNatural::NONE));
    ksng->addButton(radioKeySigNatBefore, int(KeySigNatural::BEFORE));
    ksng->addButton(radioKeySigNatAfter, int(KeySigNatural::AFTER));

    QButtonGroup* ctg = new QButtonGroup(this);
    ctg->addButton(clefTab1, int(ClefType::TAB));
    ctg->addButton(clefTab2, int(ClefType::TAB_SERIF));

    QButtonGroup* fbAlign = new QButtonGroup(this);
    fbAlign->addButton(radioFBTop, 0);
    fbAlign->addButton(radioFBBottom, 1);

    QButtonGroup* fbStyle = new QButtonGroup(this);
    fbStyle->addButton(radioFBModern, 0);
    fbStyle->addButton(radioFBHistoric, 1);

    const char* styles[] = {
        QT_TRANSLATE_NOOP("EditStyleBase", "Continuous"),
        QT_TRANSLATE_NOOP("EditStyleBase", "Dashed"),
        QT_TRANSLATE_NOOP("EditStyleBase", "Dotted"),
        QT_TRANSLATE_NOOP("EditStyleBase", "Dash-dotted"),
        QT_TRANSLATE_NOOP("EditStyleBase", "Dash-dot-dotted")
    };
    int dta = 1;
    voltaLineStyle->clear();
    ottavaLineStyle->clear();
    pedalLineStyle->clear();
    for (const char* p : styles) {
        QString trs = qApp->translate("EditStyleBase", p);
        voltaLineStyle->addItem(trs, dta);
        ottavaLineStyle->addItem(trs, dta);
        pedalLineStyle->addItem(trs, dta);
        ++dta;
    }

    styleWidgets = {
        //   idx                --- showPercent      --- widget          --- resetButton
        { Sid::figuredBassAlignment,    false, fbAlign,                 0 },
        { Sid::figuredBassStyle,        false, fbStyle,                 0 },
        { Sid::figuredBassFontSize,     false, doubleSpinFBSize,        0 },
        { Sid::figuredBassYOffset,      false, doubleSpinFBVertPos,     0 },
        { Sid::figuredBassLineHeight,   true,  spinFBLineHeight,        0 },
        { Sid::tabClef,                 false, ctg,                     0 },
        { Sid::keySigNaturals,          false, ksng,                    0 },
        { Sid::voltaLineStyle,          false, voltaLineStyle,          resetVoltaLineStyle },
        { Sid::ottavaLineStyle,         false, ottavaLineStyle,         resetOttavaLineStyle },
        { Sid::pedalLineStyle,          false, pedalLineStyle,          resetPedalLineStyle },

        { Sid::staffUpperBorder,        false, staffUpperBorder,        resetStaffUpperBorder },
        { Sid::staffLowerBorder,        false, staffLowerBorder,        resetStaffLowerBorder },
        { Sid::staffDistance,           false, staffDistance,           resetStaffDistance },
        { Sid::akkoladeDistance,        false, akkoladeDistance,        resetAkkoladeDistance },
        { Sid::minSystemDistance,       false, minSystemDistance,       resetMinSystemDistance },
        { Sid::maxSystemDistance,       false, maxSystemDistance,       resetMaxSystemDistance },

        { Sid::lyricsPlacement,         false, lyricsPlacement,         resetLyricsPlacement },
        { Sid::lyricsPosAbove,          false, lyricsPosAbove,          resetLyricsPosAbove },
        { Sid::lyricsPosBelow,          false, lyricsPosBelow,          resetLyricsPosBelow },
        { Sid::lyricsMinTopDistance,    false, lyricsMinTopDistance,    resetLyricsMinTopDistance },
        { Sid::lyricsMinBottomDistance, false, lyricsMinBottomDistance, resetLyricsMinBottomDistance },
        { Sid::lyricsMinDistance,       false, lyricsMinDistance,       resetLyricsMinDistance },
        { Sid::lyricsLineHeight,        true,  lyricsLineHeight,        resetLyricsLineHeight },
        { Sid::lyricsDashMinLength,     false, lyricsDashMinLength,     resetLyricsDashMinLength },
        { Sid::lyricsDashMaxLength,     false, lyricsDashMaxLength,     resetLyricsDashMaxLength },
        { Sid::lyricsDashMaxDistance,   false, lyricsDashMaxDistance,   resetLyricsDashMaxDistance },
        { Sid::lyricsDashForce,         false, lyricsDashForce,         resetLyricsDashForce },
        { Sid::lyricsAlignVerseNumber,  false, lyricsAlignVerseNumber,  resetLyricsAlignVerseNumber },
        { Sid::lyricsLineThickness,     false, lyricsLineThickness,     resetLyricsLineThickness },
        { Sid::lyricsMelismaPad,        false, lyricsMelismaPad,        resetLyricsMelismaPad },
        { Sid::lyricsMelismaAlign,      false, lyricsMelismaAlign,      resetLyricsMelismaAlign },
        { Sid::lyricsDashPad,           false, lyricsDashPad,           resetLyricsDashPad },
        { Sid::lyricsDashLineThickness, false, lyricsDashLineThickness, resetLyricsDashLineThickness },
        { Sid::lyricsDashYposRatio,     false, lyricsDashYposRatio,     resetLyricsDashYposRatio },

        { Sid::systemFrameDistance,     false, systemFrameDistance,     resetSystemFrameDistance },
        { Sid::frameSystemDistance,     false, frameSystemDistance,     resetFrameSystemDistance },
        { Sid::minMeasureWidth,         false, minMeasureWidth_2,       resetMinMeasureWidth },
        { Sid::measureSpacing,          false, measureSpacing,          resetMeasureSpacing },

        { Sid::barWidth,                false, barWidth,                resetBarWidth },
        { Sid::endBarWidth,             false, endBarWidth,             resetEndBarWidth },
        { Sid::endBarDistance,          false, endBarDistance,          resetEndBarDistance },
        { Sid::doubleBarWidth,          false, doubleBarWidth,          resetDoubleBarWidth },
        { Sid::doubleBarDistance,       false, doubleBarDistance,       resetDoubleBarDistance },
        { Sid::repeatBarlineDotSeparation, false, repeatBarlineDotSeparation, resetRepeatBarlineDotSeparation },

        { Sid::barGraceDistance,        false, barGraceDistance,        resetBarGraceDistance },
        { Sid::chordExtensionMag,       false, extensionMag,            resetExtensionMag },
        { Sid::chordExtensionAdjust,    false, extensionAdjust,         resetExtensionAdjust },
        { Sid::chordModifierMag,        false, modifierMag,             resetModifierMag },
        { Sid::chordModifierAdjust,     false, modifierAdjust,          resetModifierAdjust },
        { Sid::useStandardNoteNames,    false, useStandardNoteNames,    0 },
        { Sid::useGermanNoteNames,      false, useGermanNoteNames,      0 },
        { Sid::useFullGermanNoteNames,  false, useFullGermanNoteNames,  0 },
        { Sid::useSolfeggioNoteNames,   false, useSolfeggioNoteNames,   0 },
        { Sid::useFrenchNoteNames,      false, useFrenchNoteNames,      0 },
        { Sid::automaticCapitalization, false, automaticCapitalization, 0 },

        { Sid::lowerCaseMinorChords,    false, lowerCaseMinorChords,    0 },

        { Sid::lowerCaseBassNotes,      false, lowerCaseBassNotes,      0 },
        { Sid::allCapsNoteNames,        false, allCapsNoteNames,        0 },
        { Sid::concertPitch,            false, concertPitch,            0 },
        { Sid::createMultiMeasureRests, false, multiMeasureRests,       0 },
        { Sid::minEmptyMeasures,        false, minEmptyMeasures,        0 },
        { Sid::minMMRestWidth,          false, minMeasureWidth,         resetMinMMRestWidth },
        { Sid::mmRestNumberPos,         false, mmRestNumberPos,         resetMMRestNumberPos },
        { Sid::mmRestNumberMaskHBar,    false, mmRestNumberMaskHBar,    resetMMRestNumberMaskHBar },
        { Sid::mmRestHBarThickness,     false, mmRestHBarThickness,     resetMMRestHBarThickness },
        { Sid::multiMeasureRestMargin,  false, multiMeasureRestMargin,  resetMultiMeasureRestMargin },
        { Sid::mmRestHBarVStrokeThickness, false, mmRestHBarVStrokeThickness, resetMMRestHBarVStrokeThickness },
        { Sid::mmRestHBarVStrokeHeight, false, mmRestHBarVStrokeHeight, resetMMRestHBarVStrokeHeight },
        { Sid::oldStyleMultiMeasureRests, false, oldStyleMultiMeasureRests, 0 },
        { Sid::mmRestOldStyleMaxMeasures, false, mmRestOldStyleMaxMeasures, resetMMRestOldStyleMaxMeasures },
        { Sid::mmRestOldStyleSpacing,   false, mmRestOldStyleSpacing,   resetMMRestOldStyleSpacing },
        { Sid::hideEmptyStaves,         false, hideEmptyStaves,         0 },
        { Sid::dontHideStavesInFirstSystem, false, dontHideStavesInFirstSystem, 0 },
        { Sid::alwaysShowBracketsWhenEmptyStavesAreHidden, false, alwaysShowBrackets, 0 },
        { Sid::hideInstrumentNameIfOneInstrument, false, hideInstrumentNameIfOneInstrument, 0 },
        { Sid::accidentalNoteDistance,  false, accidentalNoteDistance,  0 },
        { Sid::accidentalDistance,      false, accidentalDistance,      0 },

        { Sid::minNoteDistance,         false, minNoteDistance,         resetMinNoteDistance },
        { Sid::barNoteDistance,         false, barNoteDistance,         resetBarNoteDistance },
        { Sid::barAccidentalDistance,   false, barAccidentalDistance,   resetBarAccidentalDistance },
        { Sid::noteBarDistance,         false, noteBarDistance,         resetNoteBarDistance },
        { Sid::clefLeftMargin,          false, clefLeftMargin,          resetClefLeftMargin },
        { Sid::keysigLeftMargin,        false, keysigLeftMargin,        resetKeysigLeftMargin },
        { Sid::timesigLeftMargin,       false, timesigLeftMargin,       resetTimesigLeftMargin },
        { Sid::midClefKeyRightMargin,   false, clefKeyRightMargin,      resetClefKeyRightMargin },
        { Sid::clefKeyDistance,         false, clefKeyDistance,         resetClefKeyDistance },
        { Sid::clefTimesigDistance,     false, clefTimesigDistance,     resetClefTimesigDistance },
        { Sid::keyTimesigDistance,      false, keyTimesigDistance,      resetKeyTimesigDistance },
        { Sid::keyBarlineDistance,      false, keyBarlineDistance,      resetKeyBarlineDistance },
        { Sid::systemHeaderDistance,    false, systemHeaderDistance,    resetSystemHeaderDistance },
        { Sid::systemHeaderTimeSigDistance,    false, systemHeaderTimeSigDistance,
          resetSystemHeaderTimeSigDistance },

        { Sid::clefBarlineDistance,     false, clefBarlineDistance,     resetClefBarlineDistance },
        { Sid::timesigBarlineDistance,  false, timesigBarlineDistance,  resetTimesigBarlineDistance },
        { Sid::staffLineWidth,          false, staffLineWidth,          resetStaffLineWidth },
        { Sid::beamWidth,               false, beamWidth,               0 },
        { Sid::beamMinLen,              false, beamMinLen,              0 },

        { Sid::hairpinPlacement,        false, hairpinPlacement,        resetHairpinPlacement },
        { Sid::hairpinPosAbove,         false, hairpinPosAbove,         resetHairpinPosAbove },
        { Sid::hairpinPosBelow,         false, hairpinPosBelow,         resetHairpinPosBelow },
        { Sid::hairpinLineWidth,        false, hairpinLineWidth,        resetHairpinLineWidth },
        { Sid::hairpinHeight,           false, hairpinHeight,           resetHairpinHeight },
        { Sid::hairpinContHeight,       false, hairpinContinueHeight,   resetHairpinContinueHeight },

        { Sid::dotNoteDistance,         false, noteDotDistance,         0 },
        { Sid::dotDotDistance,          false, dotDotDistance,          0 },
        { Sid::stemWidth,               false, stemWidth,               0 },
        { Sid::ledgerLineWidth,         false, ledgerLineWidth,         0 },
        { Sid::ledgerLineLength,        false, ledgerLineLength,        0 },
        { Sid::shortStemProgression,    false, shortStemProgression,    0 },
        { Sid::shortestStem,            false, shortestStem,            0 },
        { Sid::ArpeggioNoteDistance,    false, arpeggioNoteDistance,    0 },
        { Sid::ArpeggioLineWidth,       false, arpeggioLineWidth,       0 },
        { Sid::ArpeggioHookLen,         false, arpeggioHookLen,         0 },
        { Sid::ArpeggioHiddenInStdIfTab,false, arpeggioHiddenInStdIfTab,0 },
        { Sid::SlurEndWidth,            false, slurEndLineWidth,        resetSlurEndLineWidth },
        { Sid::SlurMidWidth,            false, slurMidLineWidth,        resetSlurMidLineWidth },
        { Sid::SlurDottedWidth,         false, slurDottedLineWidth,     resetSlurDottedLineWidth },
        { Sid::SlurMinDistance,         false, slurMinDistance,         resetSlurMinDistance },
        { Sid::MinTieLength,            false, minTieLength,            resetMinTieLength },
        { Sid::bracketWidth,            false, bracketWidth,            0 },
        { Sid::bracketDistance,         false, bracketDistance,         0 },
        { Sid::akkoladeWidth,           false, akkoladeWidth,           0 },
        { Sid::akkoladeBarDistance,     false, akkoladeBarDistance,     0 },
        { Sid::dividerLeft,             false, dividerLeft,             0 },
        { Sid::dividerLeftX,            false, dividerLeftX,            0 },
        { Sid::dividerLeftY,            false, dividerLeftY,            0 },
        { Sid::dividerRight,            false, dividerRight,            0 },
        { Sid::dividerRightX,           false, dividerRightX,           0 },
        { Sid::dividerRightY,           false, dividerRightY,           0 },
        { Sid::propertyDistanceHead,    false, propertyDistanceHead,    resetPropertyDistanceHead },
        { Sid::propertyDistanceStem,    false, propertyDistanceStem,    resetPropertyDistanceStem },
        { Sid::propertyDistance,        false, propertyDistance,        resetPropertyDistance },
        { Sid::voltaPosAbove,           false, voltaPosAbove,           resetVoltaPosAbove },
        { Sid::voltaHook,               false, voltaHook,               resetVoltaHook },
        { Sid::voltaLineWidth,          false, voltaLineWidth,          resetVoltaLineWidth },

        { Sid::ottavaPosAbove,          false, ottavaPosAbove,          resetOttavaPosAbove },
        { Sid::ottavaPosBelow,          false, ottavaPosBelow,          resetOttavaPosBelow },
        { Sid::ottavaHookAbove,         false, ottavaHookAbove,         resetOttavaHookAbove },
        { Sid::ottavaHookBelow,         false, ottavaHookBelow,         resetOttavaHookBelow },
        { Sid::ottavaLineWidth,         false, ottavaLineWidth,         resetOttavaLineWidth },

        { Sid::pedalPlacement,          false, pedalLinePlacement,      resetPedalLinePlacement },
        { Sid::pedalPosAbove,           false, pedalLinePosAbove,       resetPedalLinePosAbove },
        { Sid::pedalPosBelow,           false, pedalLinePosBelow,       resetPedalLinePosBelow },
        { Sid::pedalLineWidth,          false, pedalLineWidth,          resetPedalLineWidth },

        { Sid::trillPlacement,          false, trillLinePlacement,      resetTrillLinePlacement },
        { Sid::trillPosAbove,           false, trillLinePosAbove,       resetTrillLinePosAbove },
        { Sid::trillPosBelow,           false, trillLinePosBelow,       resetTrillLinePosBelow },

        { Sid::vibratoPlacement,        false, vibratoLinePlacement,      resetVibratoLinePlacement },
        { Sid::vibratoPosAbove,         false, vibratoLinePosAbove,       resetVibratoLinePosAbove },
        { Sid::vibratoPosBelow,         false, vibratoLinePosBelow,       resetVibratoLinePosBelow },

        { Sid::harmonyFretDist,         false, harmonyFretDist,         0 },
        { Sid::minHarmonyDistance,      false, minHarmonyDistance,      0 },
        { Sid::maxHarmonyBarDistance,   false, maxHarmonyBarDistance,   0 },
        { Sid::maxChordShiftAbove,      false, maxChordShiftAbove,      resetMaxChordShiftAbove },
        { Sid::maxChordShiftBelow,      false, maxChordShiftBelow,      resetMaxChordShiftBelow },

        { Sid::tupletVHeadDistance,     false, tupletVHeadDistance,     resetTupletVHeadDistance },
        { Sid::tupletVStemDistance,     false, tupletVStemDistance,     resetTupletVStemDistance },
        { Sid::tupletStemLeftDistance,  false, tupletStemLeftDistance,  resetTupletStemLeftDistance },
        { Sid::tupletStemRightDistance, false, tupletStemRightDistance, resetTupletStemRightDistance },
        { Sid::tupletNoteLeftDistance,  false, tupletNoteLeftDistance,  resetTupletNoteLeftDistance },
        { Sid::tupletNoteRightDistance, false, tupletNoteRightDistance, resetTupletNoteRightDistance },
        { Sid::tupletBracketWidth,      false, tupletBracketWidth,      resetTupletBracketWidth },
        { Sid::tupletBracketHookHeight, false, tupletBracketHookHeight, resetTupletBracketHookHeight },
        { Sid::tupletDirection,         false, tupletDirection,         resetTupletDirection },
        { Sid::tupletNumberType,        false, tupletNumberType,        resetTupletNumberType },
        { Sid::tupletBracketType,       false, tupletBracketType,       resetTupletBracketType },
        { Sid::tupletMaxSlope,          false, tupletMaxSlope,          resetTupletMaxSlope },
        { Sid::tupletOufOfStaff,        false, tupletOutOfStaff,        0 },

        { Sid::repeatBarTips,            false, showRepeatBarTips,            resetShowRepeatBarTips },
        { Sid::startBarlineSingle,       false, showStartBarlineSingle,       resetShowStartBarlineSingle },
        { Sid::startBarlineMultiple,     false, showStartBarlineMultiple,     resetShowStartBarlineMultiple },
        { Sid::dividerLeftSym,           false, dividerLeftSym,               0 },
        { Sid::dividerRightSym,          false, dividerRightSym,              0 },

        { Sid::showMeasureNumber,        false, showMeasureNumber,            0 },
        { Sid::showMeasureNumberOne,     false, showFirstMeasureNumber,       0 },
        { Sid::measureNumberInterval,    false, intervalMeasureNumber,        0 },
        { Sid::measureNumberSystem,      false, showEverySystemMeasureNumber, 0 },
        { Sid::measureNumberAllStaves,   false, showAllStavesMeasureNumber,   0 },
        { Sid::measureNumberVPlacement,  false, measureNumberVPlacement,      resetMeasureNumberVPlacement },
        { Sid::measureNumberHPlacement,  false, measureNumberHPlacement,      resetMeasureNumberHPlacement },

        { Sid::beamDistance,             true,  beamDistance,                 0 },
        { Sid::beamNoSlope,              false, beamNoSlope,                  0 },
        { Sid::graceNoteMag,             true,  graceNoteSize,                resetGraceNoteSize },
        { Sid::smallStaffMag,            true,  smallStaffSize,               resetSmallStaffSize },
        { Sid::smallNoteMag,             true,  smallNoteSize,                resetSmallNoteSize },
        { Sid::smallClefMag,             true,  smallClefSize,                resetSmallClefSize },
        { Sid::lastSystemFillLimit,      true,  lastSystemFillThreshold,      resetLastSystemFillThreshold },
        { Sid::genClef,                  false, genClef,                      0 },
        { Sid::genKeysig,                false, genKeysig,                    0 },
        { Sid::genCourtesyTimesig,       false, genCourtesyTimesig,           0 },
        { Sid::genCourtesyKeysig,        false, genCourtesyKeysig,            0 },
        { Sid::genCourtesyClef,          false, genCourtesyClef,              0 },
        { Sid::swingRatio,               false, swingBox,                     0 },
        { Sid::chordsXmlFile,            false, chordsXmlFile,                0 },
        { Sid::dotMag,                   true,  dotMag,                       0 },
        { Sid::articulationMag,          true,  articulationMag,              resetArticulationMag },
        { Sid::shortenStem,              false, shortenStem,                  0 },
        { Sid::showHeader,               false, showHeader,                   0 },
        { Sid::headerFirstPage,          false, showHeaderFirstPage,          0 },
        { Sid::headerOddEven,            false, headerOddEven,                0 },
        { Sid::evenHeaderL,              false, evenHeaderL,                  0 },
        { Sid::evenHeaderC,              false, evenHeaderC,                  0 },
        { Sid::evenHeaderR,              false, evenHeaderR,                  0 },
        { Sid::oddHeaderL,               false, oddHeaderL,                   0 },
        { Sid::oddHeaderC,               false, oddHeaderC,                   0 },
        { Sid::oddHeaderR,               false, oddHeaderR,                   0 },
        { Sid::showFooter,               false, showFooter,                   0 },
        { Sid::footerFirstPage,          false, showFooterFirstPage,          0 },
        { Sid::footerOddEven,            false, footerOddEven,                0 },
        { Sid::evenFooterL,              false, evenFooterL,                  0 },
        { Sid::evenFooterC,              false, evenFooterC,                  0 },
        { Sid::evenFooterR,              false, evenFooterR,                  0 },
        { Sid::oddFooterL,               false, oddFooterL,                   0 },
        { Sid::oddFooterC,               false, oddFooterC,                   0 },
        { Sid::oddFooterR,               false, oddFooterR,                   0 },

        { Sid::ottavaNumbersOnly,        false, ottavaNumbersOnly,            resetOttavaNumbersOnly },
        { Sid::capoPosition,             false, capoPosition,                 0 },
        { Sid::fretNumMag,               true,  fretNumMag,                   0 },
        { Sid::fretNumPos,               false, fretNumGroup,                 0 },
        { Sid::fretY,                    false, fretY,                        0 },
        { Sid::barreLineWidth,           false, barreLineWidth,               0 },
        { Sid::fretMag,                  false, fretMag,                      0 },
        { Sid::fretDotSize,              false, fretDotSize,                  0 },
        { Sid::fretStringSpacing,        false, fretStringSpacing,            0 },
        { Sid::fretFretSpacing,          false, fretFretSpacing,              0 },
        { Sid::maxFretShiftAbove,        false, maxFretShiftAbove,            resetMaxFretShiftAbove },
        { Sid::maxFretShiftBelow,        false, maxFretShiftBelow,            resetMaxFretShiftBelow },
        { Sid::scaleBarlines,            false, scaleBarlines,                resetScaleBarlines },
        { Sid::crossMeasureValues,       false, crossMeasureValues,           0 },

        { Sid::MusicalSymbolFont,        false, musicalSymbolFont,            0 },
        { Sid::MusicalTextFont,          false, musicalTextFont,              0 },
        { Sid::autoplaceHairpinDynamicsDistance, false, autoplaceHairpinDynamicsDistance,
          resetAutoplaceHairpinDynamicsDistance },

        { Sid::dynamicsPlacement,       false, dynamicsPlacement,          resetDynamicsPlacement },
        { Sid::dynamicsPosAbove,        false, dynamicsPosAbove,           resetDynamicsPosAbove },
        { Sid::dynamicsPosBelow,        false, dynamicsPosBelow,           resetDynamicsPosBelow },
        { Sid::dynamicsMinDistance,     false, dynamicsMinDistance,        resetDynamicsMinDistance },

        { Sid::tempoPlacement,          false, tempoTextPlacement,          resetTempoTextPlacement },
        { Sid::tempoPosAbove,           false, tempoTextPosAbove,           resetTempoTextPosAbove },
        { Sid::tempoPosBelow,           false, tempoTextPosBelow,           resetTempoTextPosBelow },
        { Sid::tempoMinDistance,        false, tempoTextMinDistance,        resetTempoTextMinDistance },

        { Sid::rehearsalMarkPlacement,   false, rehearsalMarkPlacement,     resetRehearsalMarkPlacement },
        { Sid::rehearsalMarkPosAbove,    false, rehearsalMarkPosAbove,      resetRehearsalMarkPosAbove },
        { Sid::rehearsalMarkPosBelow,    false, rehearsalMarkPosBelow,      resetRehearsalMarkPosBelow },
        { Sid::rehearsalMarkMinDistance, false, rehearsalMarkMinDistance,   resetRehearsalMarkMinDistance },

        { Sid::autoplaceVerticalAlignRange, false, autoplaceVerticalAlignRange, resetAutoplaceVerticalAlignRange },
        { Sid::minVerticalDistance,         false, minVerticalDistance,         resetMinVerticalDistance },
        { Sid::textLinePlacement,           false, textLinePlacement,           resetTextLinePlacement },
        { Sid::textLinePosAbove,            false, textLinePosAbove,            resetTextLinePosAbove },
        { Sid::textLinePosBelow,            false, textLinePosBelow,            resetTextLinePosBelow },

        { Sid::fermataPosAbove,         false, fermataPosAbove,       resetFermataPosAbove },
        { Sid::fermataPosBelow,         false, fermataPosBelow,       resetFermataPosBelow },
        { Sid::fermataMinDistance,      false, fermataMinDistance,    resetFermataMinDistance },

        { Sid::staffTextPlacement,      false, staffTextPlacement,    resetStaffTextPlacement },
        { Sid::staffTextPosAbove,       false, staffTextPosAbove,     resetStaffTextPosAbove },
        { Sid::staffTextPosBelow,       false, staffTextPosBelow,     resetStaffTextPosBelow },
        { Sid::staffTextMinDistance,    false, staffTextMinDistance,  resetStaffTextMinDistance },

        { Sid::bendLineWidth,     false, bendLineWidth,     resetBendLineWidth },
        { Sid::bendArrowWidth,    false, bendArrowWidth,    resetBendArrowWidth },
    };

    for (QComboBox* cb : std::vector<QComboBox*> {
        lyricsPlacement, textLinePlacement, hairpinPlacement, pedalLinePlacement,
        trillLinePlacement, vibratoLinePlacement, dynamicsPlacement,
        tempoTextPlacement, staffTextPlacement, rehearsalMarkPlacement,
        measureNumberVPlacement
    }) {
        cb->clear();
        cb->addItem(tr("Above"), int(Placement::ABOVE));
        cb->addItem(tr("Below"), int(Placement::BELOW));
    }

    measureNumberHPlacement->clear();
    measureNumberHPlacement->addItem(tr("Left"),   int(HPlacement::LEFT));
    measureNumberHPlacement->addItem(tr("Center"), int(HPlacement::CENTER));
    measureNumberHPlacement->addItem(tr("Right"),  int(HPlacement::RIGHT));

    autoplaceVerticalAlignRange->clear();
    autoplaceVerticalAlignRange->addItem(tr("Segment"), int(VerticalAlignRange::SEGMENT));
    autoplaceVerticalAlignRange->addItem(tr("Measure"), int(VerticalAlignRange::MEASURE));
    autoplaceVerticalAlignRange->addItem(tr("System"),  int(VerticalAlignRange::SYSTEM));

    tupletNumberType->clear();
    tupletNumberType->addItem(tr("Number"), int(TupletNumberType::SHOW_NUMBER));
    tupletNumberType->addItem(tr("Ratio"), int(TupletNumberType::SHOW_RELATION));
    tupletNumberType->addItem(tr("None", "no tuplet number type"), int(TupletNumberType::NO_TEXT));

    tupletBracketType->clear();
    tupletBracketType->addItem(tr("Automatic"), int(TupletBracketType::AUTO_BRACKET));
    tupletBracketType->addItem(tr("Bracket"), int(TupletBracketType::SHOW_BRACKET));
    tupletBracketType->addItem(tr("None", "no tuplet bracket type"), int(TupletBracketType::SHOW_NO_BRACKET));

    pageList->setCurrentRow(0);
    accidentalsGroup->setVisible(false);   // disable, not yet implemented

    musicalSymbolFont->clear();
    int idx = 0;
    for (auto i : ScoreFont::scoreFonts()) {
        musicalSymbolFont->addItem(i.name(), i.name());
        ++idx;
    }

    static const SymId ids[] = {
        SymId::systemDivider, SymId::systemDividerLong, SymId::systemDividerExtraLong
    };
    for (SymId id : ids) {
        const QString& un = Sym::id2userName(id);
        const char* n  = Sym::id2name(id);
        dividerLeftSym->addItem(un,  QVariant(QString(n)));
        dividerRightSym->addItem(un, QVariant(QString(n)));
    }

    // figured bass init
    QList<QString> fbFontNames = FiguredBass::fontNames();
    for (const QString& family: fbFontNames) {
        comboFBFont->addItem(family);
    }
    comboFBFont->setCurrentIndex(0);
    connect(comboFBFont, SIGNAL(currentIndexChanged(int)), SLOT(on_comboFBFont_currentIndexChanged(int)));

    // keep in sync with implementation in Page::replaceTextMacros (page.cpp)
    // jumping thru hoops here to make the job of translators easier, yet have a nice display
    QString toolTipHeaderFooter
        = QString("<html><head></head><body><p><b>")
          + tr("Special symbols in header/footer")
          + QString("</b></p>")
          + QString("<table><tr><td>$p</td><td>-</td><td><i>")
          + tr("Page number, except on first page")
          + QString("</i></td></tr><tr><td>$N</td><td>-</td><td><i>")
          + tr("Page number, if there is more than one page")
          + QString("</i></td></tr><tr><td>$P</td><td>-</td><td><i>")
          + tr("Page number, on all pages")
          + QString("</i></td></tr><tr><td>$n</td><td>-</td><td><i>")
          + tr("Number of pages")
          + QString("</i></td></tr><tr><td>$f</td><td>-</td><td><i>")
          + tr("File name")
          + QString("</i></td></tr><tr><td>$F</td><td>-</td><td><i>")
          + tr("File path+name")
          + QString("</i></td></tr><tr><td>$i</td><td>-</td><td><i>")
          + tr("Part name, except on first page")
          + QString("</i></td></tr><tr><td>$I</td><td>-</td><td><i>")
          + tr("Part name, on all pages")
          + QString("</i></td></tr><tr><td>$d</td><td>-</td><td><i>")
          + tr("Current date")
          + QString("</i></td></tr><tr><td>$D</td><td>-</td><td><i>")
          + tr("Creation date")
          + QString("</i></td></tr><tr><td>$m</td><td>-</td><td><i>")
          + tr("Last modification time")
          + QString("</i></td></tr><tr><td>$M</td><td>-</td><td><i>")
          + tr("Last modification date")
          + QString("</i></td></tr><tr><td>$C</td><td>-</td><td><i>")
          + tr("Copyright, on first page only")
          + QString("</i></td></tr><tr><td>$c</td><td>-</td><td><i>")
          + tr("Copyright, on all pages")
          + QString("</i></td></tr><tr><td>$$</td><td>-</td><td><i>")
          + tr("The $ sign itself")
          + QString("</i></td></tr><tr><td>$:tag:</td><td>-</td><td><i>")
          + tr("Metadata tag, see below")
          + QString("</i></td></tr></table><p>")
          + tr("Available metadata tags and their current values")
          + QString("<br />")
          + tr("(in File > Score Properties…):")
          + QString("</p><table>");

    // show all tags for current score/part, see also Score::init()
    QList<QMap<QString, QString> > tags = globalContext()->currentNotation()->styleEditor()->metaTags();
    for (const QMap<QString, QString>& tag: tags) {
        QMapIterator<QString, QString> i(tag);
        while (i.hasNext()) {
            i.next();
            toolTipHeaderFooter += QString("<tr><td>%1</td><td>-</td><td>%2</td></tr>").arg(i.key()).arg(i.value());
        }
    }

    toolTipHeaderFooter += QString("</table></body></html>");
    showHeader->setToolTip(toolTipHeaderFooter);
    showHeader->setToolTipDuration(5000);   // leaving the default value of -1 calculates the duration automatically and it takes too long
    showFooter->setToolTip(toolTipHeaderFooter);
    showFooter->setToolTipDuration(5000);

    connect(buttonBox,           SIGNAL(clicked(QAbstractButton*)), SLOT(buttonClicked(QAbstractButton*)));
    connect(headerOddEven,       SIGNAL(toggled(bool)),             SLOT(toggleHeaderOddEven(bool)));
    connect(footerOddEven,       SIGNAL(toggled(bool)),             SLOT(toggleFooterOddEven(bool)));
    connect(chordDescriptionFileButton, SIGNAL(clicked()),          SLOT(selectChordDescriptionFile()));
    connect(chordsStandard,      SIGNAL(toggled(bool)),             SLOT(setChordStyle(bool)));
    connect(chordsJazz,          SIGNAL(toggled(bool)),             SLOT(setChordStyle(bool)));
    connect(chordsCustom,        SIGNAL(toggled(bool)),             SLOT(setChordStyle(bool)));
    connect(chordsXmlFile,       SIGNAL(toggled(bool)),             SLOT(setChordStyle(bool)));
    connect(chordDescriptionFile,&QLineEdit::editingFinished,       [=]() { setChordStyle(true); });
    //chordDescriptionFile->setEnabled(false);

//    chordDescriptionFileButton->setIcon(IconCode::Code::OPEN_FILE);

    connect(swingOff,            SIGNAL(toggled(bool)),             SLOT(setSwingParams(bool)));
    connect(swingEighth,         SIGNAL(toggled(bool)),             SLOT(setSwingParams(bool)));
    connect(swingSixteenth,      SIGNAL(toggled(bool)),             SLOT(setSwingParams(bool)));

    connect(concertPitch,        SIGNAL(toggled(bool)),             SLOT(concertPitchToggled(bool)));
    connect(lyricsDashMinLength, SIGNAL(valueChanged(double)),      SLOT(lyricsDashMinLengthValueChanged(double)));
    connect(lyricsDashMaxLength, SIGNAL(valueChanged(double)),      SLOT(lyricsDashMaxLengthValueChanged(double)));
    connect(minSystemDistance,   SIGNAL(valueChanged(double)),      SLOT(systemMinDistanceValueChanged(double)));
    connect(maxSystemDistance,   SIGNAL(valueChanged(double)),      SLOT(systemMaxDistanceValueChanged(double)));

    QSignalMapper* mapper  = new QSignalMapper(this);       // reset style signals
    QSignalMapper* mapper2 = new QSignalMapper(this);       // value change signals

    for (const StyleWidget& sw : styleWidgets) {
        const char* type = MStyle::valueType(sw.idx);

        if (!strcmp("Direction", type)) {
            QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
            fillComboBoxDirection(cb);
        }
        if (sw.reset) {
//            sw.reset->setIcon(IconCode::Code::ACCOUNT);
            connect(sw.reset, SIGNAL(clicked()), mapper, SLOT(map()));
            mapper->setMapping(sw.reset, int(sw.idx));
        }
        if (qobject_cast<QSpinBox*>(sw.widget)) {
            connect(qobject_cast<QSpinBox*>(sw.widget), SIGNAL(valueChanged(int)), mapper2, SLOT(map()));
        } else if (qobject_cast<QDoubleSpinBox*>(sw.widget)) {
            connect(qobject_cast<QDoubleSpinBox*>(sw.widget), SIGNAL(valueChanged(double)), mapper2, SLOT(map()));
        } else if (qobject_cast<QFontComboBox*>(sw.widget)) {
            connect(qobject_cast<QFontComboBox*>(sw.widget), SIGNAL(currentFontChanged(const QFont&)), mapper2,
                    SLOT(map()));
        } else if (qobject_cast<QComboBox*>(sw.widget)) {
            connect(qobject_cast<QComboBox*>(sw.widget), SIGNAL(currentIndexChanged(int)), mapper2, SLOT(map()));
        } else if (qobject_cast<QRadioButton*>(sw.widget)) {
            connect(qobject_cast<QRadioButton*>(sw.widget), SIGNAL(toggled(bool)), mapper2, SLOT(map()));
        } else if (qobject_cast<QPushButton*>(sw.widget)) {
            connect(qobject_cast<QPushButton*>(sw.widget), SIGNAL(toggled(bool)), mapper2, SLOT(map()));
        } else if (qobject_cast<QToolButton*>(sw.widget)) {
            connect(qobject_cast<QToolButton*>(sw.widget), SIGNAL(toggled(bool)), mapper2, SLOT(map()));
        } else if (qobject_cast<QGroupBox*>(sw.widget)) {
            connect(qobject_cast<QGroupBox*>(sw.widget), SIGNAL(toggled(bool)), mapper2, SLOT(map()));
        } else if (qobject_cast<QCheckBox*>(sw.widget)) {
            connect(qobject_cast<QCheckBox*>(sw.widget), SIGNAL(stateChanged(int)), mapper2, SLOT(map()));
        } else if (qobject_cast<QTextEdit*>(sw.widget)) {
            connect(qobject_cast<QTextEdit*>(sw.widget), SIGNAL(textChanged()), mapper2, SLOT(map()));
        } else if (qobject_cast<QButtonGroup*>(sw.widget)) {
            connect(qobject_cast<QButtonGroup*>(sw.widget), SIGNAL(buttonClicked(int)), mapper2, SLOT(map()));
        } else if (qobject_cast<AlignSelect*>(sw.widget)) {
            connect(qobject_cast<AlignSelect*>(sw.widget), SIGNAL(alignChanged(Align)), mapper2, SLOT(map()));
        } else if (qobject_cast<OffsetSelect*>(sw.widget)) {
            connect(qobject_cast<OffsetSelect*>(sw.widget), SIGNAL(offsetChanged(const QPointF&)), mapper2,
                    SLOT(map()));
        } else if (FontStyleSelect* fontStyle = qobject_cast<FontStyleSelect*>(sw.widget)) {
            connect(fontStyle, &FontStyleSelect::fontStyleChanged, mapper2, QOverload<>::of(&QSignalMapper::map));
        } else {
            qFatal("unhandled gui widget type %s valueType %s",
                   sw.widget->metaObject()->className(),
                   MStyle::valueName(sw.idx)
                   );
        }

        mapper2->setMapping(sw.widget, int(sw.idx));
    }

    int topBottomMargin = automaticCapitalization->rect().height() - 5;//preferences.getInt(PREF_UI_THEME_FONTSIZE);
    topBottomMargin /= 2;
    topBottomMargin = topBottomMargin > 4 ? topBottomMargin - 4 : 0;
    automaticCapitalization->layout()->setContentsMargins(9, topBottomMargin, 9, topBottomMargin);

    connect(mapper,  SIGNAL(mapped(int)), SLOT(resetStyleValue(int)));
    connect(mapper2, SIGNAL(mapped(int)), SLOT(valueChanged(int)));
    textStyles->clear();
    for (auto ss : allTextStyles()) {
        QListWidgetItem* item = new QListWidgetItem(
            globalContext()->currentNotation()->styleEditor()->textStyleUserName(ss));
        item->setData(Qt::UserRole, int(ss));
        textStyles->addItem(item);
    }

    textStyleFrameType->clear();
    textStyleFrameType->addItem(tr("None", "no frame for text"), int(FrameType::NO_FRAME));
    textStyleFrameType->addItem(tr("Rectangle"), int(FrameType::SQUARE));
    textStyleFrameType->addItem(tr("Circle"), int(FrameType::CIRCLE));

//    resetTextStyleName->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleName, &QToolButton::clicked, [=]() { resetUserStyleName(); });
    connect(styleName, &QLineEdit::textEdited, [=]() { editUserStyleName(); });
    connect(styleName, &QLineEdit::editingFinished, [=]() { endEditUserStyleName(); });

    // font face
//    resetTextStyleFontFace->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFontFace, &QToolButton::clicked,
            [=]() { resetTextStyle(Pid::FONT_FACE); }
            );
    connect(textStyleFontFace, &QFontComboBox::currentFontChanged,
            [=]() { textStyleValueChanged(Pid::FONT_FACE, QVariant(textStyleFontFace->currentFont().family())); }
            );

    // font size
//    resetTextStyleFontSize->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFontSize, &QToolButton::clicked,
            [=]() { resetTextStyle(Pid::FONT_SIZE); }
            );
    connect(textStyleFontSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=]() { textStyleValueChanged(Pid::FONT_SIZE, QVariant(textStyleFontSize->value())); }
            );

    // font style
//    resetTextStyleFontStyle->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFontStyle, &QToolButton::clicked,
            [=]() { resetTextStyle(Pid::FONT_STYLE); }
            );
    connect(textStyleFontStyle, &FontStyleSelect::fontStyleChanged,
            [=]() { textStyleValueChanged(Pid::FONT_STYLE, QVariant(int(textStyleFontStyle->fontStyle()))); }
            );

    // align
//    resetTextStyleAlign->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleAlign, &QToolButton::clicked, [=]() { resetTextStyle(Pid::ALIGN); });
    connect(textStyleAlign, &AlignSelect::alignChanged,
            [=]() { textStyleValueChanged(Pid::ALIGN, QVariant::fromValue(textStyleAlign->align())); }
            );

    // offset
//    resetTextStyleOffset->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleOffset, &QToolButton::clicked, [=]() { resetTextStyle(Pid::OFFSET); });
    connect(textStyleOffset, &OffsetSelect::offsetChanged,
            [=]() { textStyleValueChanged(Pid::OFFSET, QVariant(textStyleOffset->offset())); }
            );

    // spatium dependent
//    resetTextStyleSpatiumDependent->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleSpatiumDependent, &QToolButton::clicked, [=]() {
        resetTextStyle(Pid::SIZE_SPATIUM_DEPENDENT);
    });
    connect(textStyleSpatiumDependent, &QCheckBox::toggled,
            [=]() { textStyleValueChanged(Pid::SIZE_SPATIUM_DEPENDENT, textStyleSpatiumDependent->isChecked()); }
            );

//    resetTextStyleFrameType->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFrameType, &QToolButton::clicked, [=]() { resetTextStyle(Pid::FRAME_TYPE); });
    connect(textStyleFrameType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=]() { textStyleValueChanged(Pid::FRAME_TYPE, textStyleFrameType->currentIndex()); }
            );

//    resetTextStyleFramePadding->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFramePadding, &QToolButton::clicked, [=]() { resetTextStyle(Pid::FRAME_PADDING); });
    connect(textStyleFramePadding, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=]() { textStyleValueChanged(Pid::FRAME_PADDING, textStyleFramePadding->value()); }
            );

//    resetTextStyleFrameBorder->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFrameBorder, &QToolButton::clicked, [=]() { resetTextStyle(Pid::FRAME_WIDTH); });
    connect(textStyleFrameBorder, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=]() { textStyleValueChanged(Pid::FRAME_WIDTH, textStyleFrameBorder->value()); }
            );

//    resetTextStyleFrameBorderRadius->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFrameBorderRadius, &QToolButton::clicked, [=]() { resetTextStyle(Pid::FRAME_ROUND); });
    connect(textStyleFrameBorderRadius, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [=]() { textStyleValueChanged(Pid::FRAME_ROUND, textStyleFrameBorderRadius->value()); }
            );

//    resetTextStyleFrameForeground->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFrameForeground, &QToolButton::clicked, [=]() { resetTextStyle(Pid::FRAME_FG_COLOR); });
    connect(textStyleFrameForeground, &Awl::ColorLabel::colorChanged,
            [=]() { textStyleValueChanged(Pid::FRAME_FG_COLOR, textStyleFrameForeground->color()); }
            );

//    resetTextStyleFrameBackground->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleFrameBackground, &QToolButton::clicked, [=]() { resetTextStyle(Pid::FRAME_BG_COLOR); });
    connect(textStyleFrameBackground, &Awl::ColorLabel::colorChanged,
            [=]() { textStyleValueChanged(Pid::FRAME_BG_COLOR, textStyleFrameBackground->color()); }
            );

//    resetTextStyleColor->setIcon(IconCode::Code::ACCOUNT);
    connect(resetTextStyleColor, &QToolButton::clicked, [=]() { resetTextStyle(Pid::COLOR); });
    connect(textStyleColor, &Awl::ColorLabel::colorChanged,
            [=]() { textStyleValueChanged(Pid::COLOR, textStyleColor->color()); }
            );

    connect(textStyles, SIGNAL(currentRowChanged(int)), SLOT(textStyleChanged(int)));
    textStyles->setCurrentRow(0);

    QRect scr = QGuiApplication::primaryScreen()->availableGeometry();
    QRect dlg = this->frameGeometry();
    isTooBig  = dlg.width() > scr.width() || dlg.height() > scr.height();
    if (isTooBig) {
        this->setMinimumSize(scr.width() / 2, scr.height() / 2);
    }
    hasShown = false;
    //MuseScore::restoreGeometry(this); TODO
}

EditStyle::EditStyle(const EditStyle& other)
    : QDialog(other.parentWidget())
{
    cs = other.cs;
}

//---------------------------------------------------------
//   PAGES
///   This is a map of element type to pages, to allow the creation of a 'Style...'
///   menu for every possible element on the score.
//---------------------------------------------------------

const std::map<ElementType, EditStylePage> EditStyle::PAGES = {
    { ElementType::SCORE,               &EditStyle::PageScore },
    { ElementType::PAGE,                &EditStyle::PagePage },
    { ElementType::MEASURE_NUMBER,      &EditStyle::PageMeasureNumbers },
    { ElementType::BRACKET,             &EditStyle::PageSystem },
    { ElementType::BRACKET_ITEM,        &EditStyle::PageSystem },
    { ElementType::CLEF,                &EditStyle::PageClefs },
    { ElementType::KEYSIG,              &EditStyle::PageAccidentals },
    { ElementType::MEASURE,             &EditStyle::PageMeasure },
    { ElementType::REST,                &EditStyle::PageRests },
    { ElementType::MMREST,              &EditStyle::PageRests },
    { ElementType::BAR_LINE,            &EditStyle::PageBarlines },
    { ElementType::NOTE,                &EditStyle::PageNotes },
    { ElementType::CHORD,               &EditStyle::PageNotes },
    { ElementType::ACCIDENTAL,          &EditStyle::PageNotes },
    { ElementType::STEM,                &EditStyle::PageNotes },
    { ElementType::STEM_SLASH,          &EditStyle::PageNotes },
    { ElementType::LEDGER_LINE,         &EditStyle::PageNotes },
    { ElementType::BEAM,                &EditStyle::PageBeams },
    { ElementType::TUPLET,              &EditStyle::PageTuplets },
    { ElementType::ARPEGGIO,            &EditStyle::PageArpeggios },
    { ElementType::SLUR,                &EditStyle::PageSlursTies },
    { ElementType::SLUR_SEGMENT,        &EditStyle::PageSlursTies },
    { ElementType::TIE,                 &EditStyle::PageSlursTies },
    { ElementType::TIE_SEGMENT,         &EditStyle::PageSlursTies },
    { ElementType::HAIRPIN,             &EditStyle::PageHairpins },
    { ElementType::HAIRPIN_SEGMENT,     &EditStyle::PageHairpins },
    { ElementType::VOLTA,               &EditStyle::PageVolta },
    { ElementType::VOLTA_SEGMENT,       &EditStyle::PageVolta },
    { ElementType::OTTAVA,              &EditStyle::PageOttava },
    { ElementType::OTTAVA_SEGMENT,      &EditStyle::PageOttava },
    { ElementType::PEDAL,               &EditStyle::PagePedal },
    { ElementType::PEDAL_SEGMENT,       &EditStyle::PagePedal },
    { ElementType::TRILL,               &EditStyle::PageTrill },
    { ElementType::TRILL_SEGMENT,       &EditStyle::PageTrill },
    { ElementType::VIBRATO,             &EditStyle::PageVibrato },
    { ElementType::VIBRATO_SEGMENT,     &EditStyle::PageVibrato },
    { ElementType::BEND,                &EditStyle::PageBend },
    { ElementType::TEXTLINE,            &EditStyle::PageTextLine },
    { ElementType::TEXTLINE_SEGMENT,    &EditStyle::PageTextLine },
    { ElementType::ARTICULATION,        &EditStyle::PageArticulationsOrnaments },
    { ElementType::FERMATA,             &EditStyle::PageFermatas },
    { ElementType::STAFF_TEXT,          &EditStyle::PageStaffText },
    { ElementType::TEMPO_TEXT,          &EditStyle::PageTempoText },
    { ElementType::LYRICS,              &EditStyle::PageLyrics },
    { ElementType::LYRICSLINE,          &EditStyle::PageLyrics },
    { ElementType::LYRICSLINE_SEGMENT,  &EditStyle::PageLyrics },
    { ElementType::DYNAMIC,             &EditStyle::PageDynamics },
    { ElementType::REHEARSAL_MARK,      &EditStyle::PageRehearsalMarks },
    { ElementType::FIGURED_BASS,        &EditStyle::PageFiguredBass },
    { ElementType::HARMONY,             &EditStyle::PageChordSymbols },
    { ElementType::FRET_DIAGRAM,        &EditStyle::PageFretboardDiagrams },
};

//---------------------------------------------------------
//   gotoElement
///   switch the page to the one related to the element `e`
//---------------------------------------------------------

void EditStyle::gotoElement(Element* e)
{
    const ElementType& t = e->type();
    const auto i = PAGES.find(t);
    if (i != PAGES.cend()) {
        QWidget* page = this->*(i->second);
        setPage(pageStack->indexOf(page));
    }
}

//---------------------------------------------------------
//   gotoElement
///   used to go to the correct page when double-clicking on a header/footer
//---------------------------------------------------------

void EditStyle::gotoHeaderFooterPage()
{
    setPage(pageStack->indexOf(PageHeaderFooter));
}

//---------------------------------------------------------
//   elementHasPage
///   check if an element has a style page related to it
//---------------------------------------------------------

bool EditStyle::elementHasPage(Element* e)
{
    const ElementType& t = e->type();
    const auto i = PAGES.find(t);
    return i != PAGES.cend();
}

//---------------------------------------------------------
//   showEvent
//---------------------------------------------------------

void EditStyle::showEvent(QShowEvent* ev)
{
    if (!hasShown && isTooBig) {
        // Add scroll bars to pageStack - this cannot be in the constructor
        // or the Header, Footer text input boxes size themselves too large.
        QScrollArea* scrollArea = new QScrollArea(splitter);
        scrollArea->setWidget(pageStack);
        hasShown = true;     // so that it only happens once
    }
    setValues();
    pageList->setFocus();
    globalContext()->currentNotation()->styleEditor()->startEdit();
    buttonApplyToAllParts->setEnabled(!globalContext()->currentNotation()->styleEditor()->isMaster());
    QWidget::showEvent(ev);
}

//---------------------------------------------------------
//   hideEvent
//---------------------------------------------------------

void EditStyle::hideEvent(QHideEvent* ev)
{
//    MuseScore::saveGeometry(this); // TODO
    QWidget::hideEvent(ev);
}

//---------------------------------------------------------
//   buttonClicked
//---------------------------------------------------------

void EditStyle::buttonClicked(QAbstractButton* b)
{
    switch (buttonBox->standardButton(b)) {
    case QDialogButtonBox::Ok:
        done(1);
        globalContext()->currentNotation()->styleEditor()->apply();
        break;
    case QDialogButtonBox::Cancel:
        done(0);
        globalContext()->currentNotation()->styleEditor()->cancel();
        break;
    case QDialogButtonBox::NoButton:
    default:
        if (b == buttonApplyToAllParts) {
            globalContext()->currentNotation()->styleEditor()->applyAllParts();
        }
        break;
    }
}

//---------------------------------------------------------
//   On comboFBFont currentIndex changed
//---------------------------------------------------------

void EditStyle::on_comboFBFont_currentIndexChanged(int index)
{
    qreal size, lineHeight;

    if (FiguredBass::fontData(index, 0, 0, &size, &lineHeight)) {
        doubleSpinFBSize->setValue(size);
        spinFBLineHeight->setValue(static_cast<int>(lineHeight * 100.0));
    }
}

//---------------------------------------------------------
//    On buttonTogglePagelist clicked
//---------------------------------------------------------

void EditStyle::on_buttonTogglePagelist_clicked()
{
    bool isVis = !pageList->isVisible();   // toggle it

    pageList->setVisible(isVis);
//    buttonTogglePagelist->setIcon(isVis ? IconCode::Code::ACCOUNT : IconCode::Code::AMBITUS);
}

//---------------------------------------------------------
//   unhandledType
//---------------------------------------------------------

static void unhandledType(const StyleWidget* sw)
{
    const char* type = MStyle::valueType(sw->idx);
    qFatal("%s <%s>: widget: %s\n", type, MStyle::valueName(sw->idx), sw->widget->metaObject()->className());
}

//---------------------------------------------------------
//   getValue
//    return current gui value
//---------------------------------------------------------

QVariant EditStyle::getValue(Sid idx)
{
    const StyleWidget& sw = styleWidget(idx);
    const char* type = MStyle::valueType(sw.idx);

    if (!strcmp("Ms::Spatium", type)) {
        QDoubleSpinBox* sb = qobject_cast<QDoubleSpinBox*>(sw.widget);
        return QVariant(Spatium(sb->value() * (sw.showPercent ? 0.01 : 1.0)));
    } else if (!strcmp("double", type)) {
        QVariant v = sw.widget->property("value");
        if (!v.isValid()) {
            unhandledType(&sw);
        }
        if (sw.showPercent) {
            v = v.toDouble() * 0.01;
        }
        return v;
    } else if (!strcmp("bool", type)) {
        QVariant v = sw.widget->property("checked");
        if (!v.isValid()) {
            unhandledType(&sw);
        }
        return v;
    } else if (!strcmp("int", type)) {
        if (qobject_cast<QComboBox*>(sw.widget)) {
            QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
            return cb->currentData().toInt();
        } else if (qobject_cast<QSpinBox*>(sw.widget)) {
            return qobject_cast<QSpinBox*>(sw.widget)->value() / (sw.showPercent ? 100 : 1);
        } else if (qobject_cast<QButtonGroup*>(sw.widget)) {
            QButtonGroup* bg = qobject_cast<QButtonGroup*>(sw.widget);
            return bg->checkedId();
        } else if (FontStyleSelect* fontStyle = qobject_cast<FontStyleSelect*>(sw.widget)) {
            return int(fontStyle->fontStyle());
        } else {
            qFatal("unhandled int");
        }
    } else if (!strcmp("QString", type)) {
        if (qobject_cast<QFontComboBox*>(sw.widget)) {
            return static_cast<QFontComboBox*>(sw.widget)->currentFont().family();
        }
        if (qobject_cast<QComboBox*>(sw.widget)) {
            QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
            return cb->currentData().toString();
        }
        if (qobject_cast<QTextEdit*>(sw.widget)) {
            QTextEdit* te = qobject_cast<QTextEdit*>(sw.widget);
            return te->toPlainText();
        }
        qFatal("getValue: unhandled widget type %s valueType %s",
               sw.widget->metaObject()->className(),
               MStyle::valueName(idx));
    } else if (!strcmp("QPointF", type)) {
        OffsetSelect* cb = qobject_cast<OffsetSelect*>(sw.widget);
        if (cb) {
            return cb->offset();
        } else {
            qFatal("unhandled QPointF");
        }
    } else if (!strcmp("Ms::Direction", type)) {
        QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
        if (cb) {
            return cb->currentIndex();
        } else {
            qFatal("unhandled Direction");
        }
    } else if (!strcmp("Ms::Align", type)) {
        AlignSelect* as = qobject_cast<AlignSelect*>(sw.widget);
        return QVariant::fromValue(as->align());
    } else {
        qFatal("EditStyle::getValue: unhandled type <%s>", type);
    }
    return QVariant();
}

//---------------------------------------------------------
//   setValues
//---------------------------------------------------------

void EditStyle::setValues()
{
    const MStyle& lstyle = globalContext()->currentNotation()->styleEditor()->style();
    for (const StyleWidget& sw : styleWidgets) {
        if (sw.widget) {
            sw.widget->blockSignals(true);
        }
        QVariant val = lstyle.value(sw.idx);
        const char* type = MStyle::valueType(sw.idx);
        if (sw.reset) {
            sw.reset->setEnabled(!lstyle.isDefault(sw.idx));
        }

        if (!strcmp("Ms::Spatium", type)) {
            if (sw.showPercent) {
                qobject_cast<QSpinBox*>(sw.widget)->setValue(int(val.value<Spatium>().val() * 100.0));
            } else {
                sw.widget->setProperty("value", val);
            }
        } else if (!strcmp("double", type)) {
            if (sw.showPercent) {
                val = QVariant(val.toDouble() * 100);
            }
            if (!sw.widget->setProperty("value", val)) {
                unhandledType(&sw);
            }
        } else if (!strcmp("bool", type)) {
            if (!sw.widget->setProperty("checked", val)) {
                unhandledType(&sw);
            }
            if (sw.idx == Sid::measureNumberSystem && !val.toBool()) {
                showIntervalMeasureNumber->setChecked(true);
            }
        } else if (!strcmp("int", type)) {
            if (qobject_cast<QComboBox*>(sw.widget)) {
                QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
                cb->setCurrentIndex(cb->findData(val));
            } else if (qobject_cast<QSpinBox*>(sw.widget)) {
                qobject_cast<QSpinBox*>(sw.widget)->setValue(val.toInt()
                                                             * (sw.showPercent ? 100 : 1));
            } else if (qobject_cast<QButtonGroup*>(sw.widget)) {
                QButtonGroup* bg = qobject_cast<QButtonGroup*>(sw.widget);
                for (auto a : bg->buttons()) {
                    if (bg->id(a) == val.toInt()) {
                        a->setChecked(true);
                        break;
                    }
                }
            } else if (FontStyleSelect* fontStyle = qobject_cast<FontStyleSelect*>(sw.widget)) {
                fontStyle->setFontStyle(FontStyle(val.toInt()));
            } else {
                unhandledType(&sw);
            }
        } else if (!strcmp("QString", type)) {
            if (qobject_cast<QFontComboBox*>(sw.widget)) {
                static_cast<QFontComboBox*>(sw.widget)->setCurrentFont(QFont(val.toString()));
            } else if (qobject_cast<QComboBox*>(sw.widget)) {
                QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
                for (int i = 0; i < cb->count(); ++i) {
                    if (cb->itemData(i) == val.toString()) {
                        cb->setCurrentIndex(i);
                        break;
                    }
                }
            } else if (qobject_cast<QTextEdit*>(sw.widget)) {
                static_cast<QTextEdit*>(sw.widget)->setPlainText(val.toString());
            } else {
                unhandledType(&sw);
            }
        } else if (!strcmp("Ms::Direction", type)) {
            QComboBox* cb = qobject_cast<QComboBox*>(sw.widget);
            if (cb) {
                cb->setCurrentIndex(int(val.value<Direction>()));
            } else {
                unhandledType(&sw);
            }
        } else if (!strcmp("Ms::Align", type)) {
            AlignSelect* as = qobject_cast<AlignSelect*>(sw.widget);
            as->setAlign(val.value<Align>());
        } else if (!strcmp("QPointF", type)) {
            OffsetSelect* as = qobject_cast<OffsetSelect*>(sw.widget);
            if (as) {
                as->setOffset(val.value<QPointF>());
            }
#if 0  // debug
            else {
                printf("no widget for QPointF <%s><%s>\n",
                       sw.widget->metaObject()->className(), MStyle::valueName(sw.idx));
            }
#endif
        } else {
            unhandledType(&sw);
        }
        if (sw.widget) {
            sw.widget->blockSignals(false);
        }
    }

    textStyleChanged(textStyles->currentRow());

    //TODO: convert the rest:

    QString unit(lstyle.value(Sid::swingUnit).toString());

    if (unit == TDuration(TDuration::DurationType::V_EIGHTH).name()) {
        swingEighth->setChecked(true);
        swingBox->setEnabled(true);
    } else if (unit == TDuration(TDuration::DurationType::V_16TH).name()) {
        swingSixteenth->setChecked(true);
        swingBox->setEnabled(true);
    } else if (unit == TDuration(TDuration::DurationType::V_ZERO).name()) {
        swingOff->setChecked(true);
        swingBox->setEnabled(false);
    }
    QString s(lstyle.value(Sid::chordDescriptionFile).toString());
    chordDescriptionFile->setText(s);
    QString cstyle(lstyle.value(Sid::chordStyle).toString());
    if (cstyle == "std") {
        chordsStandard->setChecked(true);
        chordDescriptionGroup->setEnabled(false);
    } else if (cstyle == "jazz") {
        chordsJazz->setChecked(true);
        chordDescriptionGroup->setEnabled(false);
    } else {
        chordsCustom->setChecked(true);
        chordDescriptionGroup->setEnabled(true);
    }
    //formattingGroup->setEnabled(lstyle.chordList()->autoAdjust());

    // figured bass
    for (int i = 0; i < comboFBFont->count(); i++) {
        if (comboFBFont->itemText(i) == lstyle.value(Sid::figuredBassFontFamily).toString()) {
            comboFBFont->setCurrentIndex(i);
            break;
        }
    }
    doubleSpinFBSize->setValue(lstyle.value(Sid::figuredBassFontSize).toDouble());
    doubleSpinFBVertPos->setValue(lstyle.value(Sid::figuredBassYOffset).toDouble());
    spinFBLineHeight->setValue(lstyle.value(Sid::figuredBassLineHeight).toDouble() * 100.0);

    QString mfont(lstyle.value(Sid::MusicalSymbolFont).toString());
    int idx = 0;
    for (const auto& i : ScoreFont::scoreFonts()) {
        if (i.name().toLower() == mfont.toLower()) {
            musicalSymbolFont->setCurrentIndex(idx);
            break;
        }
        ++idx;
    }
    musicalTextFont->blockSignals(true);
    musicalTextFont->clear();
    // CAUTION: the second element, the itemdata, is a font family name!
    // It's also stored in score file as the musicalTextFont
    musicalTextFont->addItem("Bravura Text", "Bravura Text");
    musicalTextFont->addItem("Emmentaler Text", "MScore Text");
    musicalTextFont->addItem("Gonville Text", "Gootville Text");
    musicalTextFont->addItem("MuseJazz Text", "MuseJazz Text");
    QString tfont(lstyle.value(Sid::MusicalTextFont).toString());
    idx = musicalTextFont->findData(tfont);
    musicalTextFont->setCurrentIndex(idx);
    musicalTextFont->blockSignals(false);

    toggleHeaderOddEven(lstyle.value(Sid::headerOddEven).toBool());
    toggleFooterOddEven(lstyle.value(Sid::footerOddEven).toBool());
}

//---------------------------------------------------------
//   selectChordDescriptionFile
//---------------------------------------------------------

void EditStyle::selectChordDescriptionFile()
{
    QString fn;// = mscore->getChordStyleFilename(true);// TODO
    if (fn.isEmpty()) {
        return;
    }
    chordDescriptionFile->setText(fn);
    setChordStyle(true);
}

//---------------------------------------------------------
//   setSwingParams
//---------------------------------------------------------

void EditStyle::setSwingParams(bool checked)
{
    if (!checked) {
        return;
    }
    QVariant val;
    if (swingOff->isChecked()) {
        val = TDuration(TDuration::DurationType::V_ZERO).name();
        swingBox->setEnabled(false);
    } else if (swingEighth->isChecked()) {
        val = TDuration(TDuration::DurationType::V_EIGHTH).name();
        swingBox->setEnabled(true);
    } else if (swingSixteenth->isChecked()) {
        val = TDuration(TDuration::DurationType::V_16TH).name();
        swingBox->setEnabled(true);
    }
    globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, Sid::swingUnit, val));

    globalContext()->currentNotation()->styleEditor()->update();
}

//---------------------------------------------------------
//   concertPitchToggled
//---------------------------------------------------------

void EditStyle::concertPitchToggled(bool checked)
{
    globalContext()->currentNotation()->styleEditor()->setConcertPitch(checked);
}

//---------------------------------------------------------
//   setChordStyle
//---------------------------------------------------------

void EditStyle::setChordStyle(bool checked)
{
    if (!checked) {
        return;
    }
    QVariant val;
    QString file;
    bool chordsXml;
    if (chordsStandard->isChecked()) {
        val  = QString("std");
        file = "chords_std.xml";
        chordsXml = false;
    } else if (chordsJazz->isChecked()) {
        val  = QString("jazz");
        file = "chords_jazz.xml";
        chordsXml = false;
    } else {
        val = QString("custom");
        chordDescriptionGroup->setEnabled(true);
        file = chordDescriptionFile->text();
        chordsXml = chordsXmlFile->isChecked();
    }
    if (val != "custom") {
        chordsXmlFile->setChecked(chordsXml);
        chordDescriptionGroup->setEnabled(false);
        chordDescriptionFile->setText(file);
    }
    globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, Sid::chordsXmlFile,
                                                                                      chordsXml));
    globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, Sid::chordStyle, val));
    if (!file.isEmpty()) {
        globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr,
                                                                                          Sid::chordDescriptionFile,
                                                                                          file));
        globalContext()->currentNotation()->styleEditor()->update();
    }
    //formattingGroup->setEnabled(cs->style().chordList()->autoAdjust());
}

//---------------------------------------------------------
//   toggleHeaderOddEven
//---------------------------------------------------------

void EditStyle::toggleHeaderOddEven(bool checked)
{
    if (!showHeader->isChecked()) {
        return;
    }
    labelEvenHeader->setEnabled(checked);
    evenHeaderL->setEnabled(checked);
    evenHeaderC->setEnabled(checked);
    evenHeaderR->setEnabled(checked);
    static QString odd  = labelOddHeader->text();    // save on 1st round
    static QString even = labelEvenHeader->text();   // save on 1st round
    if (checked) {
        labelOddHeader->setText(odd);     // restore
    } else {
        labelOddHeader->setText(odd + "\n" + even);     // replace
    }
    return;
}

//---------------------------------------------------------
//   toggleFooterOddEven
//---------------------------------------------------------

void EditStyle::toggleFooterOddEven(bool checked)
{
    if (!showFooter->isChecked()) {
        return;
    }
    labelEvenFooter->setEnabled(checked);
    evenFooterL->setEnabled(checked);
    evenFooterC->setEnabled(checked);
    evenFooterR->setEnabled(checked);
    static QString odd  = labelOddFooter->text();    // save on 1st round
    static QString even = labelEvenFooter->text();   // save on 1st round
    if (checked) {
        labelOddFooter->setText(odd);     // restore
    } else {
        labelOddFooter->setText(odd + "\n" + even);     // replace
    }
    return;
}

//---------------------------------------------------------
//   lyricsDashMin/MaxLengthValueChanged
//
//    Ensure lyricsDashMinLength <= lyricsDashMaxLength
//---------------------------------------------------------

void EditStyle::lyricsDashMaxLengthValueChanged(double val)
{
    double otherVal = lyricsDashMinLength->value();
    if (otherVal > val) {
        lyricsDashMaxLength->setValue(otherVal);
    }
}

void EditStyle::lyricsDashMinLengthValueChanged(double val)
{
    double otherVal = lyricsDashMaxLength->value();
    if (otherVal < val) {
        lyricsDashMinLength->setValue(otherVal);
    }
}

//---------------------------------------------------------
//   systemMin/MaxDistanceValueChanged
//
//    Ensure minSystemDistance <= maxSystemDistance
//---------------------------------------------------------

void EditStyle::systemMaxDistanceValueChanged(double val)
{
    double otherVal = minSystemDistance->value();
    if (otherVal > val) {
        maxSystemDistance->setValue(otherVal);
    }
}

void EditStyle::systemMinDistanceValueChanged(double val)
{
    double otherVal = maxSystemDistance->value();
    if (otherVal < val) {
        minSystemDistance->setValue(otherVal);
    }
}

//---------------------------------------------------------
//   setPage
//---------------------------------------------------------

void EditStyle::setPage(int row)
{
    pageList->setCurrentRow(row);
}

//---------------------------------------------------------
//   styleWidget
//---------------------------------------------------------

const StyleWidget& EditStyle::styleWidget(Sid idx) const
{
    for (const StyleWidget& sw : styleWidgets) {
        if (sw.idx == idx) {
            return sw;
        }
    }
#if (!defined (_MSCVER) && !defined (_MSC_VER))
    __builtin_unreachable();
#else
    // The MSVC __assume() optimizer hint is similar, though not identical, to __builtin_unreachable()
    __assume(0);
#endif
}

//---------------------------------------------------------
//   valueChanged
//---------------------------------------------------------

void EditStyle::valueChanged(int i)
{
    Style style = globalContext()->currentNotation()->styleEditor()->style();

    Sid idx       = (Sid)i;
    QVariant val  = getValue(idx);
    bool setValue = false;
    if (idx == Sid::MusicalSymbolFont && optimizeStyleCheckbox->isChecked()) {
        ScoreFont* scoreFont = ScoreFont::fontFactory(val.toString());
        if (scoreFont) {
            for (auto j : scoreFont->engravingDefaults()) {
                globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, j.first,
                                                                                                  j.second));
            }
            // fix values, the distances are defined different in MuseScore
            globalContext()->currentNotation()->styleEditor()->changeStyle(
                new ChangeStyleVal(nullptr, Sid::endBarDistance,
                                   style.value(Sid::endBarDistance).toDouble()
                                   + (style.value(Sid::barWidth).toDouble()
                                      + style.value(Sid::endBarWidth).toDouble()) * .5)
                );
            globalContext()->currentNotation()->styleEditor()->changeStyle(
                new ChangeStyleVal(nullptr, Sid::doubleBarDistance,
                                   style.value(Sid::doubleBarDistance).toDouble()
                                   + (style.value(Sid::barWidth).toDouble()
                                   + style.value(Sid::endBarWidth).toDouble()) * .5)
                );
            // guess the repeat dot width = spatium * .3
            globalContext()->currentNotation()->styleEditor()->changeStyle(
                new ChangeStyleVal(nullptr, Sid::repeatBarlineDotSeparation,
                                   style.value(Sid::repeatBarlineDotSeparation).toDouble()
                                   + (style.value(Sid::barWidth).toDouble() + .3) * .5)
                );

            // adjust mmrest, which is not in engravingDefaults
            // TODO: create generalized method for setting style vals based on font
            if (scoreFont->name() == "Bravura") {
                cs->undo(new ChangeStyleVal(cs, Sid::mmRestHBarThickness, 1.0));
                cs->undo(new ChangeStyleVal(cs, Sid::multiMeasureRestMargin, 3.0));
            } else {
                cs->undo(new ChangeStyleVal(cs, Sid::mmRestHBarThickness,
                                            MScore::defaultStyle().value(Sid::mmRestHBarThickness)));
                cs->undo(new ChangeStyleVal(cs, Sid::multiMeasureRestMargin,
                                            MScore::defaultStyle().value(Sid::multiMeasureRestMargin)));
            }
//                  if (scoreFont->textEnclosureThickness()) {
//                        TextStyle ts = cs->textStyle(TextStyleType::REHEARSAL_MARK);
//                        ts.setFrameWidth(Spatium(scoreFont->textEnclosureThickness()));
//TODO                        cs->undo(new ChangeTextStyle(cs, ts));
//                        }
        }
        setValue = true;
    }
    globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, idx, val));
    globalContext()->currentNotation()->styleEditor()->update();
    if (setValue) {
        setValues();
    }
    const StyleWidget& sw = styleWidget(idx);
    if (sw.reset) {
        sw.reset->setEnabled(!globalContext()->currentNotation()->styleEditor()->style().isDefault(idx));
    }
}

//---------------------------------------------------------
//   resetStyleValue
//---------------------------------------------------------

void EditStyle::resetStyleValue(int i)
{
    Sid idx = (Sid)i;
    globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, idx,
                                                                                      MScore::defaultStyle().value(idx)));
    setValues();
    globalContext()->currentNotation()->styleEditor()->update();
}

//---------------------------------------------------------
//   textStyleChanged
//---------------------------------------------------------

void EditStyle::textStyleChanged(int row)
{
    Style style = globalContext()->currentNotation()->styleEditor()->style();

    Tid tid = Tid(textStyles->item(row)->data(Qt::UserRole).toInt());
    const TextStyle* ts = textStyle(tid);

    for (const StyledProperty& a : *ts) {
        switch (a.pid) {
        case Pid::FONT_FACE: {
            QVariant val = style.value(a.sid);
            textStyleFontFace->setCurrentFont(QFont(val.toString()));
            resetTextStyleFontFace->setEnabled(val != MScore::defaultStyle().value(a.sid));
        }
        break;

        case Pid::FONT_SIZE:
            textStyleFontSize->setValue(style.value(a.sid).toDouble());
            resetTextStyleFontSize->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::FONT_STYLE:
            textStyleFontStyle->setFontStyle(FontStyle(style.value(a.sid).toInt()));
            resetTextStyleFontStyle->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::ALIGN:
            textStyleAlign->setAlign(style.value(a.sid).value<Align>());
            resetTextStyleAlign->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::OFFSET:
            textStyleOffset->setOffset(style.value(a.sid).toPointF());
            resetTextStyleOffset->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::SIZE_SPATIUM_DEPENDENT: {
            QVariant val = style.value(a.sid);
            textStyleSpatiumDependent->setChecked(val.toBool());
            resetTextStyleSpatiumDependent->setEnabled(val != MScore::defaultStyle().value(a.sid));
            textStyleOffset->setSuffix(val.toBool() ? tr("sp") : tr("mm"));
        }
        break;

        case Pid::FRAME_TYPE:
            textStyleFrameType->setCurrentIndex(style.value(a.sid).toInt());
            resetTextStyleFrameType->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            frameWidget->setEnabled(style.value(a.sid).toInt() != 0);             // disable if no frame
            break;

        case Pid::FRAME_PADDING:
            textStyleFramePadding->setValue(style.value(a.sid).toDouble());
            resetTextStyleFramePadding->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::FRAME_WIDTH:
            textStyleFrameBorder->setValue(style.value(a.sid).toDouble());
            resetTextStyleFrameBorder->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::FRAME_ROUND:
            textStyleFrameBorderRadius->setValue(style.value(a.sid).toDouble());
            resetTextStyleFrameBorderRadius->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::FRAME_FG_COLOR:
            textStyleFrameForeground->setColor(style.value(a.sid).value<QColor>());
            resetTextStyleFrameForeground->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::FRAME_BG_COLOR:
            textStyleFrameBackground->setColor(style.value(a.sid).value<QColor>());
            resetTextStyleFrameBackground->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        case Pid::COLOR:
            textStyleColor->setColor(style.value(a.sid).value<QColor>());
            resetTextStyleColor->setEnabled(style.value(a.sid) != MScore::defaultStyle().value(a.sid));
            break;

        default:
            break;
        }
    }
    styleName->setText(globalContext()->currentNotation()->styleEditor()->textStyleUserName(tid));
    styleName->setEnabled(int(tid) >= int(Tid::USER1));
    resetTextStyleName->setEnabled(styleName->text() != textStyleUserName(tid));
}

//---------------------------------------------------------
//   textStyleValueChanged
//---------------------------------------------------------

void EditStyle::textStyleValueChanged(Pid pid, QVariant value)
{
    Tid tid = Tid(textStyles->item(textStyles->currentRow())->data(Qt::UserRole).toInt());
    const TextStyle* ts = textStyle(tid);

    for (const StyledProperty& a : *ts) {
        if (a.pid == pid) {
            globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, a.sid, value));
            break;
        }
    }
    textStyleChanged(textStyles->currentRow());       // update GUI (reset buttons)
    globalContext()->currentNotation()->styleEditor()->update();
}

//---------------------------------------------------------
//   resetTextStyle
//---------------------------------------------------------

void EditStyle::resetTextStyle(Pid pid)
{
    Tid tid = Tid(textStyles->item(textStyles->currentRow())->data(Qt::UserRole).toInt());
    const TextStyle* ts = textStyle(tid);

    for (const StyledProperty& a : *ts) {
        if (a.pid == pid) {
            globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, a.sid,
                                                                                              MScore::defaultStyle().
                                                                                              value(a.sid)));
            break;
        }
    }
    textStyleChanged(textStyles->currentRow());       // update GUI
    globalContext()->currentNotation()->styleEditor()->update();
}

//---------------------------------------------------------
//   editUserStyleName
//---------------------------------------------------------

void EditStyle::editUserStyleName()
{
    int row = textStyles->currentRow();
    Tid tid = Tid(textStyles->item(row)->data(Qt::UserRole).toInt());
    textStyles->item(row)->setText(styleName->text());
    resetTextStyleName->setEnabled(styleName->text() != textStyleUserName(tid));
}

//---------------------------------------------------------
//   endEditUserStyleName
//---------------------------------------------------------

void EditStyle::endEditUserStyleName()
{
    int row = textStyles->currentRow();
    Tid tid = Tid(textStyles->item(row)->data(Qt::UserRole).toInt());
    int idx = int(tid) - int(Tid::USER1);
    if ((idx < 0) || (idx > 5)) {
        qDebug("User style index %d outside of range [0,5].", idx);
        return;
    }
    Sid sid[] = { Sid::user1Name, Sid::user2Name, Sid::user3Name, Sid::user4Name, Sid::user5Name, Sid::user6Name };
    QString name = styleName->text();
    globalContext()->currentNotation()->styleEditor()->changeStyle(new ChangeStyleVal(nullptr, sid[idx], name));
    if (name == "") {
        name = textStyleUserName(tid);
        styleName->setText(name);
        textStyles->item(row)->setText(name);
        resetTextStyleName->setEnabled(false);
    }
//    MuseScoreCore::mscoreCore->updateInspector(); TODO
}

//---------------------------------------------------------
//   resetUserStyleName
//---------------------------------------------------------

void EditStyle::resetUserStyleName()
{
    styleName->clear();
    endEditUserStyleName();
}