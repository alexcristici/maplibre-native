package org.maplibre.android.testapp.activity.maplayout

import android.content.Context
import android.os.Bundle
import android.view.*
import android.widget.*
import androidx.appcompat.app.ActionBarDrawerToggle
import androidx.appcompat.app.AppCompatActivity
import androidx.drawerlayout.widget.DrawerLayout
import com.google.android.material.floatingactionbutton.FloatingActionButton
import org.maplibre.android.log.Logger
import org.maplibre.android.maps.*
import org.maplibre.android.maps.MapLibreMap.OnCameraMoveListener
import org.maplibre.android.maps.MapLibreMap.OnFpsChangedListener
import org.maplibre.android.maps.renderer.MapRenderer
import org.maplibre.android.style.layers.Layer
import org.maplibre.android.style.layers.Property
import org.maplibre.android.style.layers.PropertyFactory
import org.maplibre.android.testapp.R
import org.maplibre.android.testapp.activity.events.ObserverActivity.Companion.TAG
import org.maplibre.android.testapp.styles.TestStyles
import timber.log.Timber
import java.util.*
import kotlin.concurrent.fixedRateTimer

/**
 * Test activity showcasing the different debug modes and allows to cycle between the default map styles.
 */
open class DebugModeActivity : AppCompatActivity(), OnMapReadyCallback, OnFpsChangedListener {
    private lateinit var mapView: MapView
    private lateinit var maplibreMap: MapLibreMap
    private var cameraMoveListener: OnCameraMoveListener? = null
    private var actionBarDrawerToggle: ActionBarDrawerToggle? = null
    private var currentStyleIndex = 0
    private var isReportFps = true
    private var isContinuousRendering = false
    private var fpsView: TextView? = null

    private val renderingMetricTracker = RenderingMetricTracker()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_debug_mode)
        setupToolbar()
        setupMapView(savedInstanceState)
        setupDebugChangeView()
        setupStyleChangeView()

        renderingMetricTracker.setReportListener { encodingTime, numDrawCalls, memBuffers ->
            Logger.i(TAG, "RenderingMetric encodingTime avg: ${encodingTime.getAvg() * 1000} | max: ${encodingTime.getMax() * 1000} | min: ${encodingTime.getMin() * 1000}")
            Logger.i(TAG, "RenderingMetric numDrawCalls avg: ${numDrawCalls.getAvg().toInt()} | max: ${numDrawCalls.getMax().toInt()} | min: ${numDrawCalls.getMin().toInt()}")
            Logger.i(TAG, "RenderingMetric memBuffers avg: ${memBuffers.getAvg().toInt()} | max: ${memBuffers.getMax().toInt()} | min: ${memBuffers.getMin().toInt()}")
        }

        renderingMetricTracker.startReports(10)
    }

    private fun setupToolbar() {
        val actionBar = supportActionBar
        if (actionBar != null) {
            supportActionBar!!.setDisplayHomeAsUpEnabled(true)
            supportActionBar!!.setHomeButtonEnabled(true)
            val drawerLayout = findViewById<DrawerLayout>(R.id.drawer_layout)
            actionBarDrawerToggle = ActionBarDrawerToggle(
                this,
                drawerLayout,
                R.string.navigation_drawer_open,
                R.string.navigation_drawer_close
            )
            actionBarDrawerToggle!!.isDrawerIndicatorEnabled = true
            actionBarDrawerToggle!!.syncState()
        }
    }

    private fun setupMapView(savedInstanceState: Bundle?) {
        val maplibreMapOptions = setupMapLibreMapOptions()
        mapView = MapView(this, maplibreMapOptions)
        (findViewById<View>(R.id.coordinator_layout) as ViewGroup).addView(mapView, 0)
        mapView.addOnDidFinishLoadingStyleListener {
            if (this::maplibreMap.isInitialized) {
                setupNavigationView(maplibreMap.style!!.layers)
            }
        }
        mapView.tag = true
        mapView.onCreate(savedInstanceState)
        mapView.getMapAsync(this)
        mapView.addOnDidFinishLoadingStyleListener { Timber.d("Style loaded") }

        mapView.addOnDidFinishRenderingFrameListener { _, stats->
            renderingMetricTracker.addFrame(stats)
        }
    }

    protected open fun setupMapLibreMapOptions(): MapLibreMapOptions {
        return MapLibreMapOptions.createFromAttributes(this, null)
    }

    override fun onMapReady(map: MapLibreMap) {
        maplibreMap = map
        maplibreMap.setStyle(
            Style.Builder().fromUri(STYLES[currentStyleIndex])
        ) { style: Style -> setupNavigationView(style.layers) }
        setupZoomView()
        setFpsView()
    }

    private fun setFpsView() {
        fpsView = findViewById(R.id.fpsView)
        maplibreMap.setOnFpsChangedListener(this)
    }

    override fun onFpsChanged(fps: Double) {
        fpsView!!.text = String.format(Locale.US, "FPS: %4.2f", fps)
    }

    private fun setupNavigationView(layerList: List<Layer>) {
        Timber.v("New style loaded with JSON: %s", maplibreMap.style!!.json)
        val adapter = LayerListAdapter(this, layerList)
        val listView = findViewById<ListView>(R.id.listView)
        listView.adapter = adapter
        listView.onItemClickListener =
            AdapterView.OnItemClickListener { parent: AdapterView<*>?, view: View?, position: Int, id: Long ->
                val clickedLayer = adapter.getItem(position)
                toggleLayerVisibility(clickedLayer)
                closeNavigationView()
            }
    }

    private fun toggleLayerVisibility(layer: Layer) {
        val isVisible = layer.visibility.getValue() == Property.VISIBLE
        layer.setProperties(
            PropertyFactory.visibility(
                if (isVisible) Property.NONE else Property.VISIBLE
            )
        )
    }

    private fun closeNavigationView() {
        val drawerLayout = findViewById<DrawerLayout>(R.id.drawer_layout)
        drawerLayout.closeDrawers()
    }

    private fun setupZoomView() {
        val textView = findViewById<TextView>(R.id.textZoom)
        maplibreMap.addOnCameraMoveListener(
            OnCameraMoveListener {
                textView.text = String.format(
                    this@DebugModeActivity.getString(
                        R.string.debug_zoom
                    ),
                    maplibreMap.cameraPosition.zoom
                )
            }.also { cameraMoveListener = it }
        )
    }

    private fun setupDebugChangeView() {
        val fabDebug = findViewById<FloatingActionButton>(R.id.fabDebug)
        fabDebug.setOnClickListener { view: View? ->
            if (this::maplibreMap.isInitialized) {
                maplibreMap.isDebugActive = !maplibreMap.isDebugActive
                Timber.d("Debug FAB: isDebug Active? %s", maplibreMap.isDebugActive)
            }
        }
    }

    private fun setupStyleChangeView() {
        val fabStyles = findViewById<FloatingActionButton>(R.id.fabStyles)
        fabStyles.setOnClickListener { view: View? ->
            if (this::maplibreMap.isInitialized) {
                currentStyleIndex++
                if (currentStyleIndex == STYLES.size) {
                    currentStyleIndex = 0
                }
                maplibreMap.setStyle(Style.Builder().fromUri(STYLES[currentStyleIndex]))
            }
        }
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        val itemId = item.itemId
        if (itemId == R.id.menu_action_toggle_report_fps) {
            isReportFps = !isReportFps
            fpsView!!.visibility = if (isReportFps) View.VISIBLE else View.GONE
            maplibreMap.setOnFpsChangedListener(if (isReportFps) this else null)
        } else if (itemId == R.id.menu_action_limit_to_30_fps) {
            mapView.setMaximumFps(30)
        } else if (itemId == R.id.menu_action_limit_to_60_fps) {
            mapView.setMaximumFps(60)
        } else if (itemId == R.id.menu_action_toggle_continuous_rendering) {
            isContinuousRendering = !isContinuousRendering
            if (isContinuousRendering) {
                mapView.setRenderingRefreshMode(MapRenderer.RenderingRefreshMode.CONTINUOUS)
            } else {
                mapView.setRenderingRefreshMode(MapRenderer.RenderingRefreshMode.WHEN_DIRTY)
            }
        }
        return actionBarDrawerToggle!!.onOptionsItemSelected(item) || super.onOptionsItemSelected(
            item
        )
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menuInflater.inflate(R.menu.menu_debug, menu)
        return true
    }

    override fun onStart() {
        super.onStart()
        mapView.onStart()
    }

    override fun onResume() {
        super.onResume()
        mapView.onResume()
    }

    override fun onPause() {
        super.onPause()
        mapView.onPause()
    }

    override fun onStop() {
        super.onStop()
        mapView.onStop()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        mapView.onSaveInstanceState(outState)
    }

    override fun onDestroy() {
        super.onDestroy()
        if (this::maplibreMap.isInitialized) {
            maplibreMap.removeOnCameraMoveListener(cameraMoveListener!!)
        }
        mapView.onDestroy()
        renderingMetricTracker.stopReports()
    }

    override fun onLowMemory() {
        super.onLowMemory()
        mapView.onLowMemory()
    }

    private class LayerListAdapter(context: Context?, layers: List<Layer>) :
        BaseAdapter() {
        private val layoutInflater: LayoutInflater
        private val layers: List<Layer>
        override fun getCount(): Int {
            return layers.size
        }

        override fun getItem(position: Int): Layer {
            return layers[position]
        }

        override fun getItemId(position: Int): Long {
            return position.toLong()
        }

        override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
            val layer = layers[position]
            var view = convertView
            if (view == null) {
                view = layoutInflater.inflate(android.R.layout.simple_list_item_2, parent, false)
                val holder = ViewHolder(
                    view.findViewById(android.R.id.text1),
                    view.findViewById(android.R.id.text2)
                )
                view.tag = holder
            }
            val holder = view!!.tag as ViewHolder
            holder.text.text = layer.javaClass.simpleName
            holder.subText.text = layer.id
            return view
        }

        private class ViewHolder(val text: TextView, val subText: TextView)

        init {
            layoutInflater = LayoutInflater.from(context)
            this.layers = layers
        }
    }

    companion object {
        private val STYLES = arrayOf(
            TestStyles.getPredefinedStyleWithFallback("Streets"),
            TestStyles.getPredefinedStyleWithFallback("Outdoor"),
            TestStyles.getPredefinedStyleWithFallback("Bright"),
            TestStyles.getPredefinedStyleWithFallback("Pastel"),
            TestStyles.getPredefinedStyleWithFallback("Satellite Hybrid"),
            TestStyles.getPredefinedStyleWithFallback("Satellite Hybrid")
        )
    }
}

class RenderingMetric {
    private var min: Double = Int.MAX_VALUE.toDouble()
    private var max: Double = Int.MIN_VALUE.toDouble()
    private var total: Double = 0.0
    private var frameCount = 0

    fun getMin(): Double {
        if (frameCount == 0)
            return 0.0
        return min
    }

    fun getMax(): Double {
        if (frameCount == 0)
            return 0.0
        return max
    }

    fun getAvg(): Double {
        if (frameCount == 0)
            return 0.0
        return total / frameCount
    }

    @Synchronized fun addFrame(value: Double) {
        if (value < min) {
            min = value
        }
        if (value > max) {
            max = value
        }
        total = total + value
        frameCount++;
    }
}

class RenderingMetricTracker {
    private var encodingTime = RenderingMetric()
    private var numDrawCalls = RenderingMetric()
    private var memBuffers = RenderingMetric()

    private var intervalEncodingTime = RenderingMetric()
    private var intervalNumDrawCalls = RenderingMetric()
    private var intervalMemBuffers = RenderingMetric()

    private var reportTimer: Timer? = null
    private var reportListener: ((RenderingMetric, RenderingMetric, RenderingMetric) -> Unit)? = null

    @Synchronized fun setReportListener(listener: ((RenderingMetric, RenderingMetric, RenderingMetric) -> Unit)?) {
        reportListener = listener
    }

    @Synchronized fun startReports(interval: Long) {
        reset()

        reportTimer = fixedRateTimer(
            name = "RenderingMetricReportTimer",
            initialDelay = interval * 1000L,
            period = interval * 1000L
        ) {
            reportListener?.invoke(intervalEncodingTime, intervalNumDrawCalls, intervalMemBuffers)
            resetInterval()
        }
    }

    @Synchronized fun stopReports() {
        reportListener?.invoke(encodingTime, numDrawCalls, memBuffers)
        reportTimer?.cancel()
        reportTimer = null
    }

    @Synchronized fun addFrame(frameStats: RenderingStats) {
        encodingTime.addFrame(frameStats.encodingTime)
        numDrawCalls.addFrame(frameStats.numDrawCalls.toDouble())
        memBuffers.addFrame(frameStats.memBuffers.toDouble())

        intervalEncodingTime.addFrame(frameStats.encodingTime)
        intervalNumDrawCalls.addFrame(frameStats.numDrawCalls.toDouble())
        intervalMemBuffers.addFrame(frameStats.memBuffers.toDouble())
    }

    private fun reset() {
        encodingTime = RenderingMetric()
        numDrawCalls = RenderingMetric()
        memBuffers = RenderingMetric()
        resetInterval()
    }

    private fun resetInterval() {
        intervalEncodingTime = RenderingMetric()
        intervalNumDrawCalls = RenderingMetric()
        intervalMemBuffers = RenderingMetric()
    }
}
