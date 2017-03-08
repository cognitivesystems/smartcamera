namespace cpp glipf


struct HsvColor {
  1: double h,
  2: double s,
  3: double v
}

struct Threshold {
  1: HsvColor lower,
  2: HsvColor upper
}

struct Rect {
  1: i16 x,
  2: i16 y,
  3: i16 w,
  4: i16 h
}

struct Target {
  1: i32 id,
  2: Rect pose
}

struct Particle {
  1: i32 id,
  2: Rect pose
}


service ThresholdContours {

  void initThresholdProcessor(1: list<Threshold> thresholds)
  list<list<Rect>> getThresholdRects()
  void initTarget(1: Target targetData)
  list<double> computeDistance(1: list<Target> targets,
                               2: list<Particle> particles)
}
