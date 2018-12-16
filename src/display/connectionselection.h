#pragma once
/************************************************************************
**
** Authors:   Ulf Hermann <ulfonk_mennhar@gmx.de> (Alve),
**            Marek Krejza <krejza@gmail.com> (Caligor)
**
** This file is part of the MMapper project.
** Maintained by Nils Schimmelmann <nschimme@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the:
** Free Software Foundation, Inc.
** 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
************************************************************************/

#ifndef CONNECTIONSELECTION_H
#define CONNECTIONSELECTION_H

#include <array>
#include <QObject>
#include <QString>
#include <QtCore>

#include "../expandoracommon/RoomRecipient.h"
#include "../expandoracommon/coordinate.h" /* Coordinate2f */
#include "../global/roomid.h"
#include "../mapdata/ExitDirection.h"
#include "../mapdata/mmapper2exit.h"

class MapFrontend;
class Room;
class RoomAdmin;
struct RoomId;

struct MouseSel final
{
    Coordinate2f pos{};
    int layer = 0;

    explicit MouseSel() = default;
    explicit MouseSel(const Coordinate2f &pos, const int layer)
        : pos{pos}
        , layer{layer}
    {}

    Coordinate getCoordinate() const { return Coordinate{pos.round(), layer}; }
    Coordinate getScaledCoordinate(const float xy_scale) const
    {
        return Coordinate{(pos * xy_scale).round(), layer};
    }
};

class ConnectionSelection : public QObject, public RoomRecipient
{
    Q_OBJECT

public:
    struct ConnectionDescriptor
    {
        const Room *room = nullptr;
        ExitDirection direction{};
    };

    explicit ConnectionSelection(MapFrontend *mf, const MouseSel &sel);
    explicit ConnectionSelection();
    ~ConnectionSelection() override;

    void setFirst(MapFrontend *mf, RoomId RoomId, ExitDirection dir);
    void setSecond(MapFrontend *mf, RoomId RoomId, ExitDirection dir);
    void setSecond(MapFrontend *mf, const MouseSel &sel);
    void removeSecond();

    ConnectionDescriptor getFirst();
    ConnectionDescriptor getSecond();

    bool isValid();
    bool isFirstValid() { return m_connectionDescriptor[0].room != nullptr; }
    bool isSecondValid() { return m_connectionDescriptor[1].room != nullptr; }

    void receiveRoom(RoomAdmin *admin, const Room *aRoom) override;

public slots:

signals:

protected:
private:
    static ExitDirection ComputeDirection(const Coordinate2f &mouse_f);

    // REVISIT: give these enum names?
    std::array<ConnectionDescriptor, 2> m_connectionDescriptor{};

    bool m_first = true;
    RoomAdmin *m_admin = nullptr;
};

#endif
