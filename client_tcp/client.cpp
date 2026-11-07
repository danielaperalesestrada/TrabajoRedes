#include <pybind11/numpy.h>
#include <pybind11/stl.h>



std::vector<py::array_t<double>> trasladar_matrices(std::string ip, int port, int client_id,std::vector<py::array_t<double>> matrices)
{
}

//asdasd
PYBIND11_MODULE(tcp, m) {
    m.def("trasladar_matrices", &trasladar_matrices,
          "Envia 4 matrices al servidor y devuelve las 4 medias");
}
