/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <QtInstanceCheckButton.hxx>

QtInstanceCheckButton::QtInstanceCheckButton(QCheckBox* pCheckBox)
    : QtInstanceWidget(pCheckBox)
    , m_pCheckBox(pCheckBox)
{
    assert(m_pCheckBox);
}

void QtInstanceCheckButton::set_active(bool bActive)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] {
        m_pCheckBox->setTristate(false);
        m_pCheckBox->setChecked(bActive);
    });
}

bool QtInstanceCheckButton::get_active() const
{
    SolarMutexGuard g;
    bool bActive;
    GetQtInstance().RunInMainThread([&] { bActive = m_pCheckBox->isChecked(); });
    return bActive;
}

void QtInstanceCheckButton::set_inconsistent(bool bInconsistent)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] {
        m_pCheckBox->setTristate(true);
        m_pCheckBox->setCheckState(bInconsistent ? Qt::PartiallyChecked : Qt::Unchecked);
    });
}

bool QtInstanceCheckButton::get_inconsistent() const
{
    SolarMutexGuard g;
    bool bInconsistent;
    GetQtInstance().RunInMainThread(
        [&] { bInconsistent = m_pCheckBox->checkState() == Qt::PartiallyChecked; });
    return bInconsistent;
}

void QtInstanceCheckButton::set_label(const OUString& rText)
{
    SolarMutexGuard g;
    GetQtInstance().RunInMainThread([&] { m_pCheckBox->setText(toQString(rText)); });
}
OUString QtInstanceCheckButton::get_label() const
{
    SolarMutexGuard g;
    OUString sLabel;
    GetQtInstance().RunInMainThread([&] { sLabel = toOUString(m_pCheckBox->text()); });
    return sLabel;
}

void QtInstanceCheckButton::QtInstanceCheckButton::set_label_wrap(bool /*bWrap*/)
{
    assert(false && "Not implemented yet");
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
