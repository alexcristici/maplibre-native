import MapLibre
import SwiftUI
import UIKit

class SimpleMapView: UIViewController, MLNMapViewDelegate {
    var mapView: MLNMapView!
    let renderingMetricTracker = RenderingMetricTracker()

    override func viewDidLoad() {
        super.viewDidLoad()

        mapView = MLNMapView(frame: view.bounds, styleURL: AMERICANA_STYLE)
        mapView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        mapView.delegate = self
        
        view.addSubview(mapView)
        
        renderingMetricTracker.setReportCallback { encodingTime, numDrawCalls, memBuffers in
            print("RenderingMetric encodingTime avg: \(encodingTime.getAvg() * 1000) | max: \(encodingTime.getMax() * 1000) | min: \(encodingTime.getMin() * 1000)")
            print("RenderingMetric numDrawCalls avg: \(Int(numDrawCalls.getAvg())) | max: \(Int(numDrawCalls.getMax())) | min: \(Int(numDrawCalls.getMin()))")
            print("RenderingMetric memBuffers avg: \(Int(memBuffers.getAvg())) | max: \(Int(memBuffers.getMax())) | min: \(Int(memBuffers.getMin()))")
        }

        renderingMetricTracker.startReports(interval: 10)
    }
    
    deinit {
        renderingMetricTracker.stopReports()
    }

    func mapViewDidFinishLoadingMap(_: MLNMapView) {
        //renderingMetricTracker.startReports(interval: 10)
    }

    func mapViewDidFinishRenderingFrame(_: MLNMapView, fullyRendered _: Bool, renderingStats: MLNRenderingStats) {
        renderingMetricTracker.addFrame(renderingStats)
    }
}

// #-example-code(SimpleMap)
struct SimpleMap: UIViewControllerRepresentable {
    typealias UIViewControllerType = SimpleMapView
    
    func makeUIViewController(context _: Context) -> SimpleMapView {
        SimpleMapView()
    }

    func updateUIViewController(_: SimpleMapView, context _: Context) {}
}

// #-end-example-code

class RenderingMetric {
    private var min: Double = Double(Int.max)
    private var max: Double = Double(Int.min)
    private var total: Double = 0.0
    private var frameCount: Int = 0

    func getMin() -> Double {
        return frameCount == 0 ? 0.0 : min
    }

    func getMax() -> Double {
        return frameCount == 0 ? 0.0 : max
    }

    func getAvg() -> Double {
        return frameCount == 0 ? 0.0 : total / Double(frameCount)
    }

    func addFrame(_ value: Double) {
        if value < min {
            min = value
        }
        if value > max {
            max = value
        }
        total += value
        frameCount += 1
    }
}

class RenderingMetricTracker {
    private var encodingTime = RenderingMetric()
    private var numDrawCalls = RenderingMetric()
    private var memBuffers = RenderingMetric()

    private var intervalEncodingTime = RenderingMetric()
    private var intervalNumDrawCalls = RenderingMetric()
    private var intervalMemBuffers = RenderingMetric()

    private var reportTimer: Timer?
    private var reportCallback: ((RenderingMetric, RenderingMetric, RenderingMetric) -> Void)?

    func setReportCallback(_ callback: ((RenderingMetric, RenderingMetric, RenderingMetric) -> Void)?) {
        self.reportCallback = callback
    }

    func startReports(interval: TimeInterval) {
        reset()

        reportTimer = Timer.scheduledTimer(withTimeInterval: interval, repeats: true) { [weak self] _ in
            guard let self = self else { return }
            self.reportCallback?(self.intervalEncodingTime, self.intervalNumDrawCalls, self.intervalMemBuffers)
            self.resetInterval()
        }
    }

    func stopReports() {
        reportCallback?(encodingTime, numDrawCalls, memBuffers)
        reportTimer?.invalidate()
        reportTimer = nil
    }

    func addFrame(_ frameStats: MLNRenderingStats) {
        encodingTime.addFrame(frameStats.encodingTime)
        numDrawCalls.addFrame(Double(frameStats.numDrawCalls))
        memBuffers.addFrame(Double(frameStats.memBuffers))

        intervalEncodingTime.addFrame(frameStats.encodingTime)
        intervalNumDrawCalls.addFrame(Double(frameStats.numDrawCalls))
        intervalMemBuffers.addFrame(Double(frameStats.memBuffers))
    }

    private func reset() {
        encodingTime = RenderingMetric()
        numDrawCalls = RenderingMetric()
        memBuffers = RenderingMetric()
        resetInterval()
    }

    private func resetInterval() {
        intervalEncodingTime = RenderingMetric()
        intervalNumDrawCalls = RenderingMetric()
        intervalMemBuffers = RenderingMetric()
    }
}
