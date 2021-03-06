/*
 *   Copyright 2009 by Alan Alpert <alan.alpert@nokia.com>
 *   Copyright 2010 by Ménard Alexis <menard@kde.org>
 *   Copyright 2010 by Marco Martin <mart@kde.org>

 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "kirigamiplugin.h"
#include "columnview.h"
#include "enums.h"
#include "desktopicon.h"
#include "settings.h"
#include "formlayoutattached.h"
#include "mnemonicattached.h"
#include "delegaterecycler.h"
#include "scenepositionattached.h"
#include "wheelhandler.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickStyle>

#include "libkirigami/platformtheme.h"

static QString s_selectedStyle;

//Q_INIT_RESOURCE(kirigami);
#ifdef KIRIGAMI_BUILD_TYPE_STATIC
#include <qrc_kirigami.cpp>
#endif

QUrl KirigamiPlugin::componentUrl(const QString &fileName) const
{
    for (const QString &style : qAsConst(m_stylesFallbackChain)) {
        const QString candidate = QStringLiteral("styles/") + style + QLatin1Char('/') + fileName;
        if (QFile::exists(resolveFilePath(candidate))) {
#ifdef KIRIGAMI_BUILD_TYPE_STATIC
            return QUrl(QStringLiteral("qrc:/org/kde/kirigami/styles/") + style + QLatin1Char('/') + fileName);
#else
            return QUrl(resolveFileUrl(candidate));
#endif
        }
    }

#ifdef KIRIGAMI_BUILD_TYPE_STATIC
            return QUrl(QStringLiteral("qrc:/org/kde/kirigami/") + fileName);
#else
    return QUrl(resolveFileUrl(fileName));
#endif
}

void KirigamiPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1String(uri) == QLatin1String("org.kde.kirigami"));
    const QString style = QQuickStyle::name();

    if (!qEnvironmentVariableIsSet("XDG_CURRENT_DESKTOP")) {
        QIcon::setThemeSearchPaths({resolveFilePath(QStringLiteral(".")), QStringLiteral(":/icons")});
        QIcon::setThemeName(QStringLiteral("breeze-internal"));
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    //org.kde.desktop.plasma is a couple of files that fall back to desktop by purpose
    if ((style.isEmpty() || style == QStringLiteral("org.kde.desktop.plasma")) && QFile::exists(resolveFilePath(QStringLiteral("/styles/org.kde.desktop")))) {
        m_stylesFallbackChain.prepend(QStringLiteral("org.kde.desktop"));
    }
#elif defined(Q_OS_ANDROID)
    if (!m_stylesFallbackChain.contains(QStringLiteral("Material"))) {
        m_stylesFallbackChain.prepend(QStringLiteral("Material"));
    }
#else // do we have an iOS specific style?
    if (!m_stylesFallbackChain.contains(QStringLiteral("Material"))) {
        m_stylesFallbackChain.prepend(QStringLiteral("Material"));
    }
#endif

    if (!style.isEmpty() && QFile::exists(resolveFilePath(QStringLiteral("/styles/") + style)) && !m_stylesFallbackChain.contains(style)) {
        m_stylesFallbackChain.prepend(style);
        //if we have plasma deps installed, use them for extra integration
        if (style == QStringLiteral("org.kde.desktop") && QFile::exists(resolveFilePath(QStringLiteral("/styles/org.kde.desktop.plasma")))) {
            m_stylesFallbackChain.prepend(QStringLiteral("org.kde.desktop.plasma"));
        }
    } else {
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
        m_stylesFallbackChain.prepend(QStringLiteral("org.kde.desktop"));
#endif
    }
    //At this point the fallback chain will be selected->org.kde.desktop->Fallback
    s_selectedStyle = m_stylesFallbackChain.first();

    qmlRegisterSingletonType<Settings>(uri, 2, 0, "Settings",
         [](QQmlEngine *e, QJSEngine*) -> QObject* {
             Settings *settings = Settings::self();
             //singleton managed internally, qml should never delete it
             e->setObjectOwnership(settings, QQmlEngine::CppOwnership);
             settings->setStyle(s_selectedStyle);
             return settings;
         }
     );

    qmlRegisterUncreatableType<ApplicationHeaderStyle>(uri, 2, 0, "ApplicationHeaderStyle", QStringLiteral("Cannot create objects of type ApplicationHeaderStyle"));

    //old legacy retrocompatible Theme
    qmlRegisterSingletonType(componentUrl(QStringLiteral("Theme.qml")), uri, 2, 0, "Theme");

    qmlRegisterSingletonType(componentUrl(QStringLiteral("Units.qml")), uri, 2, 0, "Units");

    qmlRegisterType(componentUrl(QStringLiteral("Action.qml")), uri, 2, 0, "Action");
    qmlRegisterType(componentUrl(QStringLiteral("AbstractApplicationHeader.qml")), uri, 2, 0, "AbstractApplicationHeader");
    qmlRegisterType(componentUrl(QStringLiteral("AbstractApplicationWindow.qml")), uri, 2, 0, "AbstractApplicationWindow");
    qmlRegisterType(componentUrl(QStringLiteral("AbstractListItem.qml")), uri, 2, 0, "AbstractListItem");
    qmlRegisterType(componentUrl(QStringLiteral("ApplicationHeader.qml")), uri, 2, 0, "ApplicationHeader");
    qmlRegisterType(componentUrl(QStringLiteral("ToolBarApplicationHeader.qml")), uri, 2, 0, "ToolBarApplicationHeader");
    qmlRegisterType(componentUrl(QStringLiteral("ApplicationWindow.qml")), uri, 2, 0, "ApplicationWindow");
    qmlRegisterType(componentUrl(QStringLiteral("BasicListItem.qml")), uri, 2, 0, "BasicListItem");
    qmlRegisterType(componentUrl(QStringLiteral("OverlayDrawer.qml")), uri, 2, 0, "OverlayDrawer");
    qmlRegisterType(componentUrl(QStringLiteral("ContextDrawer.qml")), uri, 2, 0, "ContextDrawer");
    qmlRegisterType(componentUrl(QStringLiteral("GlobalDrawer.qml")), uri, 2, 0, "GlobalDrawer");
    qmlRegisterType(componentUrl(QStringLiteral("Heading.qml")), uri, 2, 0, "Heading");
    qmlRegisterType(componentUrl(QStringLiteral("Separator.qml")), uri, 2, 0, "Separator");
    qmlRegisterType(componentUrl(QStringLiteral("PageRow.qml")), uri, 2, 0, "PageRow");

    qmlRegisterType<DesktopIcon>(uri, 2, 0, "Icon");

    qmlRegisterType(componentUrl(QStringLiteral("Label.qml")), uri, 2, 0, "Label");
    //TODO: uncomment for 2.3 release
    //qmlRegisterTypeNotAvailable(uri, 2, 3, "Label", "Label type not supported anymore, use QtQuick.Controls.Label 2.0 instead");
    qmlRegisterType(componentUrl(QStringLiteral("OverlaySheet.qml")), uri, 2, 0, "OverlaySheet");
    qmlRegisterType(componentUrl(QStringLiteral("Page.qml")), uri, 2, 0, "Page");
    qmlRegisterType(componentUrl(QStringLiteral("ScrollablePage.qml")), uri, 2, 0, "ScrollablePage");
    qmlRegisterType(componentUrl(QStringLiteral("SplitDrawer.qml")), uri, 2, 0, "SplitDrawer");
    qmlRegisterType(componentUrl(QStringLiteral("SwipeListItem.qml")), uri, 2, 0, "SwipeListItem");

    //2.1
    qmlRegisterType(componentUrl(QStringLiteral("AbstractItemViewHeader.qml")), uri, 2, 1, "AbstractItemViewHeader");
    qmlRegisterType(componentUrl(QStringLiteral("ItemViewHeader.qml")), uri, 2, 1, "ItemViewHeader");
    qmlRegisterType(componentUrl(QStringLiteral("AbstractApplicationItem.qml")), uri, 2, 1, "AbstractApplicationItem");
    qmlRegisterType(componentUrl(QStringLiteral("ApplicationItem.qml")), uri, 2, 1, "ApplicationItem");

    //2.2
    //Theme changed from a singleton to an attached property
    qmlRegisterUncreatableType<Kirigami::PlatformTheme>(uri, 2, 2, "Theme", QStringLiteral("Cannot create objects of type Theme, use it as an attached property"));

    //2.3
    qmlRegisterType(componentUrl(QStringLiteral("FormLayout.qml")), uri, 2, 3, "FormLayout");
    qmlRegisterUncreatableType<FormLayoutAttached>(uri, 2, 3, "FormData", QStringLiteral("Cannot create objects of type FormData, use it as an attached property"));
    qmlRegisterUncreatableType<MnemonicAttached>(uri, 2, 3, "MnemonicData", QStringLiteral("Cannot create objects of type MnemonicData, use it as an attached property"));

    //2.4
    qmlRegisterType(componentUrl(QStringLiteral("AbstractCard.qml")), uri, 2, 4, "AbstractCard");
    qmlRegisterType(componentUrl(QStringLiteral("Card.qml")), uri, 2, 4, "Card");
    qmlRegisterType(componentUrl(QStringLiteral("CardsListView.qml")), uri, 2, 4, "CardsListView");
    qmlRegisterType(componentUrl(QStringLiteral("CardsGridView.qml")), uri, 2, 4, "CardsGridView");
    qmlRegisterType(componentUrl(QStringLiteral("CardsLayout.qml")), uri, 2, 4, "CardsLayout");
    qmlRegisterType(componentUrl(QStringLiteral("InlineMessage.qml")), uri, 2, 4, "InlineMessage");
    qmlRegisterUncreatableType<MessageType>(uri, 2, 4, "MessageType", QStringLiteral("Cannot create objects of type MessageType"));
    qmlRegisterType<DelegateRecycler>(uri, 2, 4, "DelegateRecycler");

    //2.5
    qmlRegisterType(componentUrl(QStringLiteral("ListItemDragHandle.qml")), uri, 2, 5, "ListItemDragHandle");
    qmlRegisterType(componentUrl(QStringLiteral("ActionToolBar.qml")), uri, 2, 5, "ActionToolBar");
    qmlRegisterUncreatableType<ScenePositionAttached>(uri, 2, 5, "ScenePosition", QStringLiteral("Cannot create objects of type ScenePosition, use it as an attached property"));

    //2.6
    qmlRegisterType(componentUrl(QStringLiteral("AboutPage.qml")), uri, 2, 6, "AboutPage");
    qmlRegisterType(componentUrl(QStringLiteral("LinkButton.qml")), uri, 2, 6, "LinkButton");
    qmlRegisterType(componentUrl(QStringLiteral("UrlButton.qml")), uri, 2, 6, "UrlButton");

    //2.7
    qmlRegisterType<ColumnView>(uri, 2, 7, "ColumnView");
    qmlRegisterType(componentUrl(QStringLiteral("ActionTextField.qml")), uri, 2, 7, "ActionTextField");

    //2.8
    qmlRegisterType(componentUrl(QStringLiteral("SearchField.qml")), uri, 2, 8, "SearchField");
    qmlRegisterType(componentUrl(QStringLiteral("PasswordField.qml")), uri, 2, 8, "PasswordField");

    //2.9
    qmlRegisterType<WheelHandler>(uri, 2, 9, "WheelHandler");
    qmlRegisterUncreatableType<KirigamiWheelEvent>(uri, 2, 9, "WheelEvent", QStringLiteral("Cannot create objects of type WheelEvent."));

    qmlProtectModule(uri, 2);
}

#include "moc_kirigamiplugin.cpp"

