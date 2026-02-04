#ifndef IMAGEGENERATIONTHREAD_H
#define IMAGEGENERATIONTHREAD_H

#include <QThread>

#include "imagerequest.h"

class Manga;

class ImageGenerationThread : public QThread
{
    Q_OBJECT
public:
    explicit ImageGenerationThread(Manga *manga);

    void startGeneration(ImageRequest *request);
    void endGeneration();
    ImageRequest *request() const;

protected:
    void run() override;

private:
    Manga *m_manga{nullptr};
    ImageRequest *m_request{nullptr};
};

#endif // IMAGEGENERATIONTHREAD_H
