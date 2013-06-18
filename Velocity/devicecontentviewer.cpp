#include "devicecontentviewer.h"
#include "ui_devicecontentviewer.h"

DeviceContentViewer::DeviceContentViewer(QStatusBar *statusBar, QWidget *parent) :
    statusBar(statusBar), QDialog(parent), ui(new Ui::DeviceContentViewer)
{
    ui->setupUi(this);

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

DeviceContentViewer::~DeviceContentViewer()
{
    for (int i = 0; i < devices.size(); i++)
        delete devices.at(i);

    delete ui;
}

void DeviceContentViewer::LoadDevices()
{
    // load all of the FATX drives as XContentDevices
    std::vector<FatxDrive*> drives = FatxDriveDetection::GetAllFatxDrives();
    for (int i = 0; i < drives.size(); i++)
    {
        XContentDevice *device = new XContentDevice(drives.at(i));
        if (!device->LoadDevice())
            continue;

        QTreeWidgetItem *deviceItem = new QTreeWidgetItem(ui->treeWidget);
        deviceItem->setText(0, QString::fromStdWString(device->GetName()));

        // set the appropriate icon
        if (device->GetDeviceType() == FatxHarddrive)
            deviceItem->setIcon(0, QIcon(QPixmap(":/Images/harddrive.png")));
        else
            deviceItem->setIcon(0, QIcon(QPixmap(":/Images/usb drive.png")));

        // load the profiles
        for (int x = 0; x < device->profiles->size(); x++)
        {
            QTreeWidgetItem *profileItem = new QTreeWidgetItem(deviceItem);
            XContentDeviceProfile profile = device->profiles->at(x);

            QString profileName = QString::fromStdWString(profile.GetName());
            if (profileName == "")
                profileName = "Unknown Profile";
            profileItem->setText(0, profileName);

            profileItem->setData(0, Qt::UserRole, QVariant::fromValue(profile.package));
            profileItem->setData(1, Qt::UserRole, QVariant(QString::fromStdString(profile.GetPathOnDevice())));
            profileItem->setData(2, Qt::UserRole, QVariant(QString::fromStdString(profile.GetRawName())));

            // set the icon to the gamerpicture
            if (profile.GetThumbnail() != NULL)
            {
                QByteArray imageBuff((char*)profile.GetThumbnail(), profile.GetThumbnailSize());
                profileItem->setIcon(0, QIcon(QPixmap::fromImage(QImage::fromData(imageBuff))));
            }
            else
            {
                profileItem->setIcon(0, QIcon(QPixmap(":/Images/HiddenAchievement.png")));
            }

            // load all the titles for this profile
            for (int y = 0; y < profile.titles.size(); y++)
            {
                QTreeWidgetItem *titleItem = new QTreeWidgetItem(profileItem);
                XContentDeviceTitle title = profile.titles.at(y);

                titleItem->setText(0, QString::fromStdWString(title.GetName()));
                QByteArray imageBuff((char*)title.GetThumbnail(), title.GetThumbnailSize());
                titleItem->setIcon(0, QIcon(QPixmap::fromImage(QImage::fromData(imageBuff))));

                // load all the saves for this title
                for (int z = 0; z < title.titleSaves.size(); z++)
                {
                    QTreeWidgetItem *saveItem = new QTreeWidgetItem(titleItem);
                    XContentDeviceItem save = title.titleSaves.at(z);

                    saveItem->setData(0, Qt::UserRole, QVariant::fromValue(save.package));
                    saveItem->setData(1, Qt::UserRole, QVariant(QString::fromStdString(save.GetPathOnDevice())));
                    saveItem->setData(2, Qt::UserRole, QVariant(QString::fromStdString(save.GetRawName())));

                    saveItem->setText(0, QString::fromStdWString(save.GetName()));

                    // set the icon to the STFS package's thumbnail
                    QByteArray imageBuff((char*)save.GetThumbnail(), save.GetThumbnailSize());
                    saveItem->setIcon(0, QIcon(QPixmap::fromImage(QImage::fromData(imageBuff))));
                }
            }
        }

        QTreeWidgetItem *sharedItemFolder = new QTreeWidgetItem(deviceItem);
        sharedItemFolder->setText(0, "Shared Items");
        sharedItemFolder->setIcon(0, QIcon(QPixmap(":/Images/FolderFileIcon.png")));

        // load the shared items
        LoadSharedItemCategory("Games", device->games, sharedItemFolder);
        LoadSharedItemCategory("DLC", device->dlc, sharedItemFolder);
        LoadSharedItemCategory("Demos", device->demos, sharedItemFolder);
        LoadSharedItemCategory("Videos", device->videos, sharedItemFolder);
        LoadSharedItemCategory("Themes", device->themes, sharedItemFolder);
        LoadSharedItemCategory("Gamer Pictures", device->gamerPictures, sharedItemFolder);
        LoadSharedItemCategory("Avatar Items", device->avatarItems, sharedItemFolder);
        LoadSharedItemCategory("System Items", device->systemItems, sharedItemFolder);

        devices.push_back(device);
    }
}

void DeviceContentViewer::LoadSharedItemCategory(QString category, std::vector<XContentDeviceSharedItem> *items, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *categoryItem = new QTreeWidgetItem(parent);
    categoryItem->setText(0, category);
    categoryItem->setIcon(0, QIcon(QPixmap(":/Images/FolderFileIcon.png")));

    // load all the category's items
    for (int i = 0; i < items->size(); i++)
    {
        QTreeWidgetItem *item = new QTreeWidgetItem(categoryItem);
        XContentDeviceItem content = items->at(i);

        item->setText(0, QString::fromStdWString(content.GetName()));

        item->setData(0, Qt::UserRole, QVariant::fromValue(content.package));
        item->setData(1, Qt::UserRole, QVariant(QString::fromStdString(content.GetPathOnDevice())));
        item->setData(2, Qt::UserRole, QVariant(QString::fromStdString(content.GetRawName())));

        // set the icon to the STFS package's thumbnail
        QByteArray imageBuff((char*)content.GetThumbnail(), content.GetThumbnailSize());
        item->setIcon(0, QIcon(QPixmap::fromImage(QImage::fromData(imageBuff))));
    }
}

void DeviceContentViewer::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    StfsPackage *package = item->data(0, Qt::UserRole).value<StfsPackage*>();
    if (package == NULL)
        return;

    if (package->metaData->contentType == Profile)
    {
        bool ok;
        ProfileEditor editor(statusBar, package, false, &ok, this);
        if (ok)
            editor.exec();
    }
    else
    {
        PackageViewer viewer(statusBar, package, QList<QAction*>(), QList<QAction*>(), this, false);
        viewer.exec();
    }
}

void DeviceContentViewer::showContextMenu(const QPoint &pos)
{
    // make sure that all the items the user has selected can be extracted
    for (int i = 0; i < ui->treeWidget->selectedItems().size(); i++)
        if (ui->treeWidget->selectedItems().at(i)->data(1, Qt::UserRole).toString() == "")
            return;

    // if the user doesn't have any items selected, then we can't extract anything
    if (ui->treeWidget->selectedItems().size() == 0)
        return;

    QPoint globalPos = ui->treeWidget->mapToGlobal(pos);
    QMenu contextMenu;

    contextMenu.addAction(QPixmap(":/Images/extract.png"), "Copy Selected to Local Disk");

    QAction *selectedItem = contextMenu.exec(globalPos);
    if (selectedItem == NULL)
        return;

    if (selectedItem->text() == "Copy Selected to Local Disk")
    {
        // get a place to save the extracted items
        if (ui->treeWidget->selectedItems().size() == 1)
        {
            QTreeWidgetItem *selectedItem = ui->treeWidget->selectedItems().at(0);

            QString savePath = QFileDialog::getSaveFileName(this, "Choose a place to save the file...", QtHelpers::DesktopLocation() + "/" + selectedItem->data(2, Qt::UserRole).toString());
            devices.at(0)->CopyFileToLocalDisk(savePath.toStdString(), selectedItem->data(1, Qt::UserRole).toString().toStdString());
        }
    }
}