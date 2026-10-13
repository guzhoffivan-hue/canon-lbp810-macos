#include "ExtendedStatus.hpp"
#include <iomanip>

namespace Capt {
    std::ostream& operator<<(std::ostream& os, const ExtendedStatus& status) {
        auto fill = os.fill();
        auto flags = os.flags();
        os << std::hex << std::setfill('0');
        os << "ExtendedStatus[Basic=0x" << std::setw(2) << static_cast<unsigned>(status.Basic)
            << " Aux=0x" << std::setw(2) << static_cast<unsigned>(status.Aux)
            << " Controller=0x" << std::setw(2) << static_cast<unsigned>(status.Controller)
            << " Engine=0x" << std::setw(4) << status.Engine
            << " PaperAvailableBits=0x" << std::setw(2) << static_cast<unsigned>(status.PaperAvailableBits)
            << std::dec
            << " Start=" << status.Start
            << " Printing=" << status.Printing
            << " Shipped=" << status.Shipped
            << " Printed=" << status.Printed
            << ']';
        os.flags(flags);
        os.fill(fill);
        return os;
    }
}
