#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkJPEGReader.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTexture.h>
#include <vtkTransform.h>
#include <vtkTextureMapToPlane.h>
#include <vtkCoordinate.h>
#include <vtkPointData.h>
#include <vtkPNGWriter.h>
#include <vtkWindowToImageFilter.h>
#include <vtkImageActor.h>
#include <vtkImageReader2.h>
#include <vtkImageReader2Factory.h>
#include <vtkImageMapper3D.h>
#include <vtkImageData.h>
#include <vector>
#include <tuple>
#include <iostream>
#include <fstream>
#include <random>

// Вспомогательная функция для вычисления барицентрических координат
std::vector<double> computeBarycentricCoordinates(double x, double y,
                                                  double x1, double y1,
                                                  double x2, double y2,
                                                  double x3, double y3) {
    double detT = (y2 - y3)*(x1 - x3) + (x3 - x2)*(y1 - y3);
    double lambda1 = ((y2 - y3)*(x - x3) + (x3 - x2)*(y - y3)) / detT;
    double lambda2 = ((y3 - y1)*(x - x3) + (x1 - x3)*(y - y3)) / detT;
    double lambda3 = 1.0 - lambda1 - lambda2;
    return {lambda1, lambda2, lambda3};
}

// Функция для извлечения 3D точек, соответствующих углам штрих-кода
std::vector<std::vector<double>> get_barcode_3d_corners(vtkPolyData* mesh, std::vector<const char*> barcodeCoords) {
    std::vector<std::vector<double>> barcode_3d_points;

    // Нормализованные границы текстурных координат штрих-кода
    double top_left[2] = { std::stod(barcodeCoords[0]), std::stod(barcodeCoords[1]) };
    double bottom_right[2] = { std::stod(barcodeCoords[2]), std::stod(barcodeCoords[3]) };

    std::cout << "Top left: (" << top_left[0] << ", " << top_left[1] << ")\n";
    std::cout << "Bottom right: (" << bottom_right[0] << ", " << bottom_right[1] << ")\n";

    vtkPoints* points = mesh->GetPoints();
    vtkDataArray* tcoords = mesh->GetPointData()->GetTCoords();

    if (!points || !tcoords) {
        std::cerr << "Error: Points or texture coordinates not found." << std::endl;
        return barcode_3d_points; // Вернуть пустой вектор, если точки или текстурные координаты не найдены
    }

    std::cout << "Number of points: " << points->GetNumberOfPoints() << std::endl;
    std::cout << "Number of cells: " << mesh->GetNumberOfCells() << std::endl;

    // Поиск треугольников, пересекающих границы штрих-кода
    for (vtkIdType i = 0; i < mesh->GetNumberOfCells(); ++i) {
        vtkCell* cell = mesh->GetCell(i);
        vtkIdList* ptIds = cell->GetPointIds();

        if (ptIds->GetNumberOfIds() != 3) {
            continue; // Пропускаем, если не треугольник
        }

        // Получаем координаты точек треугольника
        double pt1[3], pt2[3], pt3[3];
        double tc1[3], tc2[3], tc3[3];
        points->GetPoint(ptIds->GetId(0), pt1);
        points->GetPoint(ptIds->GetId(1), pt2);
        points->GetPoint(ptIds->GetId(2), pt3);
        tcoords->GetTuple(ptIds->GetId(0), tc1);
        tcoords->GetTuple(ptIds->GetId(1), tc2);
        tcoords->GetTuple(ptIds->GetId(2), tc3);

        std::cout << "Triangle " << i << ":\n";
        std::cout << "  Point 1: (" << pt1[0] << ", " << pt1[1] << ", " << pt1[2] << "), Texture Coords: (" << tc1[0] << ", " << tc1[1] << ")\n";
        std::cout << "  Point 2: (" << pt2[0] << ", " << pt2[1] << ", " << pt2[2] << "), Texture Coords: (" << tc2[0] << ", " << tc2[1] << ")\n";
        std::cout << "  Point 3: (" << pt3[0] << ", " << pt3[1] << ", " << pt3[2] << "), Texture Coords: (" << tc3[0] << ", " << tc3[1] << ")\n";

        // Проверка, попадают ли текстурные координаты в границы штрих-кода
        bool intersects = false;
        for (int j = 0; j < 3; ++j) {
            double* tc = (j == 0) ? tc1 : (j == 1) ? tc2 : tc3;
            if (tc[0] >= top_left[0] && tc[0] <= bottom_right[0]) {
                intersects = true;
                break;
            }
        }

        std::cout << "  Intersects with barcode bounding box: " << (intersects ? "Yes" : "No") << std::endl;

        if (intersects) {
            // Находим барицентрические координаты углов штрих-кода
            std::vector<double> barcodeCorners = {
                    top_left[0], top_left[1],
                    bottom_right[0], top_left[1],
                    bottom_right[0], bottom_right[1],
                    top_left[0], bottom_right[1]
            };

            for (size_t j = 0; j < barcodeCorners.size(); j += 2) {
                double bx = barcodeCorners[j];
                double by = barcodeCorners[j + 1];
                auto baryCoords = computeBarycentricCoordinates(bx, by, tc1[0], tc1[1], tc2[0], tc2[1], tc3[0], tc3[1]);

                // Проверка валидности барицентрических координат
                if (baryCoords[0] < 0 || baryCoords[0] > 1 || baryCoords[1] < 0 || baryCoords[1] > 1 || baryCoords[2] < 0 || baryCoords[2] > 1) {
                    std::cout << "  Skipping invalid barycentric coordinates.\n";
                    continue;
                }

                // Интерполируем координаты в 3D пространстве
                double px = baryCoords[0] * pt1[0] + baryCoords[1] * pt2[0] + baryCoords[2] * pt3[0];
                double py = baryCoords[0] * pt1[1] + baryCoords[1] * pt2[1] + baryCoords[2] * pt3[1];
                double pz = baryCoords[0] * pt1[2] + baryCoords[1] * pt2[2] + baryCoords[2] * pt3[2];

                barcode_3d_points.push_back({px, py, pz});
                std::cout << "  Barcode corner " << (j / 2) * 2 << ": (" << px << ", " << py << ", " << pz << ")\n";
            }
        }
    }

    return barcode_3d_points;
}

// Function to transform 3D coordinates to 2D screen coordinates
std::vector<std::vector<double>> display_compute(const std::vector<std::vector<double>>& points, vtkRenderer* renderer, const std::tuple<int, int>& img_size) {
    std::vector<std::vector<double>> points_display;
    vtkNew<vtkCoordinate> coord;
    coord->SetCoordinateSystemToWorld();

    for (const auto& point : points) {
        coord->SetValue(point.data());
        double* display_coordinates = coord->GetComputedDoubleDisplayValue(renderer);

        int img_height = std::get<1>(img_size);
        points_display.push_back({display_coordinates[0], img_height - display_coordinates[1]});
    }

    return points_display;
}

// Функция для записи координат в JSON-формат
void SaveCoordinatesAsJSON(const std::vector<std::vector<double>>& coordinates, const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Не удалось открыть файл для записи." << std::endl;
        return;
    }

    outFile << "{" << std::endl;
    outFile << "  \"all_points_x\": [";
    for (size_t i = 0; i < coordinates.size(); ++i) {
        outFile << coordinates[i][0];
        if (i < coordinates.size() - 1)
            outFile << ", ";
    }
    outFile << "]," << std::endl;

    outFile << "  \"all_points_y\": [";
    for (size_t i = 0; i < coordinates.size(); ++i) {
        outFile << coordinates[i][1];
        if (i < coordinates.size() - 1)
            outFile << ", ";
    }
    outFile << "]" << std::endl;
    outFile << "}" << std::endl;

    outFile.close();
}

std::string generateRandomString(size_t length) {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string randomString;
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    for(size_t i = 0; i < length; ++i) {
        randomString += characters[distribution(generator)];
    }

    return randomString;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

int main(int argc, char* argv[]) {
    const char* modelFilePath = argv[1]; // Путь к 3D модели
    const char* backgroundImageFilePath = argv[2]; // Путь к фоновому изображению
    std::vector<const char*> barcodeCoords = { argv[3], argv[4], argv[5], argv[6] };

    vtkNew<vtkOBJReader> objReader;
    objReader->SetFileName("mesh.obj");
    objReader->Update();

    vtkNew<vtkTexture> texture;
    vtkNew<vtkJPEGReader> jpegReader;
    jpegReader->SetFileName(modelFilePath);
    texture->SetInputConnection(jpegReader->GetOutputPort());

    texture->InterpolateOn();
    texture->RepeatOff();  // Отключаем повторение текстуры

    vtkNew<vtkTextureMapToPlane> texturePlane;
    texturePlane->SetInputConnection(objReader->GetOutputPort());
    texturePlane->AutomaticPlaneGenerationOn();

    // Apply the texture coordinates from the plane to the mesh
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(texturePlane->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->SetTexture(texture);
    actor->GetProperty()->BackfaceCullingOff();  // Отключаем отсечение задних граней
    actor->GetProperty()->EdgeVisibilityOff();   // Отключаем отображение линий сетки
    actor->GetProperty()->SetRepresentationToSurface(); // Отображаем только поверхность

    vtkNew<vtkTransform> transform;
    transform->Translate(-5, 0, 0); // Adjust the translation values as necessary
    actor->SetUserTransform(transform);

    vtkNew<vtkRenderer> renderer;
    renderer->SetBackground(0.1, 0.1, 0.1); // Темный фон

    // Create a camera and set its position and orientation
    vtkNew<vtkCamera> camera;
    camera->SetPosition(-7, -16, -40); // Position the camera #changable
    camera->SetFocalPoint(0, 0, 0); // Look at the center of the object #changable
    camera->SetViewUp(0, -1, 0); // Set the view up vector #changable

    renderer->SetActiveCamera(camera); // Add the camera to the renderer

    // Load background image
    vtkNew<vtkImageReader2Factory> readerFactory;
    vtkImageReader2* imageReader = readerFactory->CreateImageReader2(backgroundImageFilePath);
    imageReader->SetFileName(backgroundImageFilePath);
    imageReader->Update();

    vtkNew<vtkImageActor> backgroundActor;
    backgroundActor->SetInputData(imageReader->GetOutput());

    // Создаем трансформацию для фоновой плоскости
    vtkNew<vtkTransform> backgroundTransform;
    backgroundTransform->PostMultiply(); // Применять масштабирование после других трансформаций
    backgroundTransform->RotateX(1); // Тот же угол поворота, что и у объекта
    backgroundActor->SetUserTransform(backgroundTransform); // Применяем трансформацию к фоновому актору

    double shiftLeft = -280; // Смещение влево; отрицательное значение перемещает влево
    double shiftUp = -180; // Смещение вверх; положительное значение перемещает вверх
    backgroundTransform->Translate(shiftLeft, shiftUp, 330.0);

    // Add background image actor first
    renderer->AddActor(actor);
    renderer->AddActor(backgroundActor);

    vtkNew<vtkRenderWindow> renderWindow;
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(800, 600);

    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);

    vtkNew<vtkInteractorStyleTrackballCamera> style;
    renderWindowInteractor->SetInteractorStyle(style);

    renderWindow->Render();

    // Get 3D points corresponding to the barcode corners
    auto barcode_3d_points = get_barcode_3d_corners(objReader->GetOutput(), barcodeCoords);

    // Transform these points to 2D screen coordinates
    auto barcode_2d_points = display_compute(barcode_3d_points, renderer, std::make_tuple(800, 600));

    // Save the rendered window to an image
    vtkNew<vtkWindowToImageFilter> windowToImageFilter;
    windowToImageFilter->SetInput(renderWindow);
    windowToImageFilter->Update();

    std::ostringstream filePath;
    auto imageName = split(backgroundImageFilePath, '/').back();
    std::string randomString = generateRandomString(4);
    filePath << "data/" << randomString << "_" << imageName; // надо добавить /.. перед data если запускать через IDE

    vtkNew<vtkPNGWriter> writer;
    writer->SetFileName(filePath.str().c_str());
    writer->SetInputConnection(windowToImageFilter->GetOutputPort());
    writer->Write();

    // Print the 2D coordinates
    for (size_t i = 0; i < barcode_2d_points.size(); ++i) {
        std::cout << "Corner " << i << ": (" << barcode_2d_points[i][0] << ", " << barcode_2d_points[i][1] << ")\n";
    }

    auto imageNameWithoutExtension = split(imageName, '.').front();

    std::ostringstream jsonFilePath;
    jsonFilePath << "data/" << randomString << "_"  << imageNameWithoutExtension << ".json";

    SaveCoordinatesAsJSON(barcode_2d_points, jsonFilePath.str());


    // renderWindowInteractor->Start();

    return EXIT_SUCCESS;
}
